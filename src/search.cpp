#include "search.hpp"
#include "eval.hpp"
#include "tt.hpp"
#include "utils.hpp"
#include <climits>
using namespace chess;

constexpr int MAX_PLY = 128; // or whatever your max search depth is

// Two killer moves per ply
static Move killerMoves[MAX_PLY][2];

// History heuristic table: [from][to]
static int historyHeuristic[64][64];

// Quiescence search with draw/mate/stalemate detection
int quiesce(Board &board, int alpha, int beta, int plyFromRoot)
{
    chess::Movelist legalMoves;
    movegen::legalmoves(legalMoves, board);

    int stand_pat = evaluateBoard(board, plyFromRoot, legalMoves);

    if (stand_pat >= beta)
        return stand_pat;

    if (stand_pat > alpha)
        alpha = stand_pat;

    for (auto move : legalMoves)
    {
        if (!board.isCapture(move))
            continue;

        board.makeMove(move);
        int score = -quiesce(board, -beta, -alpha, plyFromRoot + 1);
        board.unmakeMove(move);

        if (score >= beta)
            return score;
        if (score > stand_pat)
            stand_pat = score;
        if (score > alpha)
            alpha = score;
    }
    return stand_pat;
}

// Negamax search (returns only score, not move)
int negamax(Board &board, int depth, int alpha, int beta,
            std::chrono::steady_clock::time_point start, double timeLimit, int plyFromRoot, bool &timedOut)
{
    using namespace std::chrono;
    if (timedOut)
        return 0;
    if (duration<double>(steady_clock::now() - start).count() > timeLimit)
    {
        timedOut = true;
        return 0;
    }

    chess::Movelist legalMoves;
    movegen::legalmoves(legalMoves, board);

    auto ttVal = ttLookup(board, depth, alpha, beta, plyFromRoot);
    if (ttVal.has_value())
        return ttVal.value().first;

    // Terminal detection
    if (board.isRepetition(1) || board.isInsufficientMaterial())
        return 0;
    if (board.isHalfMoveDraw())
        return 0;
    if (legalMoves.empty())
    {
        if (board.sideToMove() == Color::WHITE)
            return board.inCheck() ? -MATE_SCORE + plyFromRoot : 0;
        else
            return board.inCheck() ? -MATE_SCORE + plyFromRoot : 0;
    }

    // null move pruningp
    if (depth >= 3 && !board.inCheck())
    {
        int nonPawnMaterial = 0;
        for (PieceType pt : {PieceType::KNIGHT, PieceType::BISHOP, PieceType::ROOK, PieceType::QUEEN})
        {
            nonPawnMaterial += MATERIAL_VALUES[(int)pt] * board.pieces(pt, board.sideToMove()).count();
        }
        if (nonPawnMaterial >= 2 * MATERIAL_VALUES[(int)PieceType::ROOK])
        {
            board.makeNullMove();
            int nullScore = -negamax(board, depth - 3, -beta, -beta + 1, start, timeLimit, plyFromRoot + 1, timedOut);
            board.unmakeNullMove();
            if (timedOut)
                return 0;
            if (nullScore >= beta)
                return beta;
        }
    }

    if (depth <= 0)
        return quiesce(board, alpha, beta, plyFromRoot + 1);

    int bestScore = INT_MIN;
    Move bestMove = Move::NULL_MOVE;
    int originalAlpha = alpha;

    orderMovesInPlace(
        board, legalMoves, plyFromRoot,
        /*hashMove=*/std::nullopt,
        std::vector<Move>{killerMoves[plyFromRoot][0], killerMoves[plyFromRoot][1]},
        historyHeuristic);

    int moveCount = 0;
    for (auto move : legalMoves)
    {
        bool isCapture = board.isCapture(move);
        bool isPromotion = move.typeOf() == Move::PROMOTION;
        bool givesCheck = board.givesCheck(move) != CheckType::NO_CHECK;
        int reduction = 0;

        // LMR: Reduce only for quiet moves, not first move, not in check, not check, not promotion, not capture
        if (depth >= 3 && moveCount > 0 && !isCapture && !isPromotion && !givesCheck && !board.inCheck())
            reduction = 1;

        board.makeMove(move);
        int score;
        if (reduction > 0)
        {
            // Reduced-depth search with null window
            score = -negamax(board, depth - 1 - reduction, -alpha - 1, -alpha, start, timeLimit, plyFromRoot + 1, timedOut);
            // If it improves alpha, re-search at full depth
            if (score > alpha)
                score = -negamax(board, depth - 1, -beta, -alpha, start, timeLimit, plyFromRoot + 1, timedOut);
        }
        else
        {
            score = -negamax(board, depth - 1, -beta, -alpha, start, timeLimit, plyFromRoot + 1, timedOut);
        }
        board.unmakeMove(move);

        if (timedOut)
            break;

        if (score > bestScore)
        {
            bestScore = score;
            bestMove = move;
        }
        if (score > alpha)
            alpha = score;
        if (alpha >= beta)
        {
            // Killer moves: only for non-captures
            if (!isCapture)
            {
                if (killerMoves[plyFromRoot][0] != move)
                {
                    killerMoves[plyFromRoot][1] = killerMoves[plyFromRoot][0];
                    killerMoves[plyFromRoot][0] = move;
                }
                // History heuristic: only for quiet moves
                historyHeuristic[move.from().index()][move.to().index()] += depth * depth;
            }
            break;
        }
        moveCount++;
    }

    ttStore(board, depth, bestMove, bestScore, originalAlpha, beta, plyFromRoot);

    return bestScore;
}

// Negamax root: finds best move and score
SearchResult negamaxRoot(Board &board, int depth, int alpha, int beta,
                         std::chrono::steady_clock::time_point start, double timeLimit, int plyFromRoot, bool &timedOut)
{
    using namespace std::chrono;
    if (timedOut)
        return {0, Move::NULL_MOVE};
    if (duration<double>(steady_clock::now() - start).count() > timeLimit)
    {
        timedOut = true;
        return {0, Move::NULL_MOVE};
    }

    chess::Movelist legalMoves;
    movegen::legalmoves(legalMoves, board);

    // Terminal detection
    if (board.isRepetition(1) || board.isInsufficientMaterial())
        return {0, Move::NULL_MOVE};
    if (board.isHalfMoveDraw())
        return {0, Move::NULL_MOVE};
    if (legalMoves.empty())
    {
        if (board.sideToMove() == Color::WHITE)
            return {board.inCheck() ? -MATE_SCORE + plyFromRoot : 0, Move::NULL_MOVE};
        else
            return {board.inCheck() ? -MATE_SCORE + plyFromRoot : 0, Move::NULL_MOVE};
    }

    int bestScore = INT_MIN;
    Move bestMove = Move::NULL_MOVE;
    int originalAlpha = alpha;

    orderMovesInPlace(
        board, legalMoves, plyFromRoot,
        /*hashMove=*/std::nullopt,
        std::vector<Move>{killerMoves[plyFromRoot][0], killerMoves[plyFromRoot][1]},
        historyHeuristic);

    for (auto move : legalMoves)
    {
        board.makeMove(move);
        int score = -negamax(board, depth - 1, -beta, -alpha, start, timeLimit, plyFromRoot + 1, timedOut);
        board.unmakeMove(move);

        if (timedOut)
            break;

        if (score > bestScore || bestMove == Move::NULL_MOVE)
        {
            bestScore = score;
            bestMove = move;
            std::cout << "info string Best move so far: " << uci::moveToUci(bestMove) << " with score " << bestScore << "\n";
        }
        if (score > alpha)
            alpha = score;
        if (alpha >= beta)
            break;
    }

    ttStore(board, depth, bestMove, bestScore, originalAlpha, beta, plyFromRoot);

    return {bestScore, bestMove};
}

Move findBestMoveIterative(Board &board, int maxDepth, double totalTimeRemaining, double increment)
{
    // Clear killer moves and history heuristic
    for (int i = 0; i < MAX_PLY; ++i)
    {
        killerMoves[i][0] = Move::NULL_MOVE;
        killerMoves[i][1] = Move::NULL_MOVE;
    }
    for (int i = 0; i < 64; ++i)
        for (int j = 0; j < 64; ++j)
            historyHeuristic[i][j] = 0;

    TT.clear();
    int moveNumber = board.fullMoveNumber();
    chess::Movelist legalMoves;
    movegen::legalmoves(legalMoves, board);

    int movesToGo = std::max(1, std::min(40, 60 - moveNumber));
    double reserve = 1.0; // Always keep at least 1 second
    double timeForMove = std::max(0.05, std::min(
                                            (totalTimeRemaining - reserve) / movesToGo + 0.5 * increment,
                                            0.5 * totalTimeRemaining)); // Never use more than 50% of remaining time

    auto start = std::chrono::steady_clock::now();

    if (legalMoves.empty())
    {
        std::cout << "info string No legal moves available\n";
        return Move::NULL_MOVE;
    }

    Move bestMove = legalMoves[0];
    for (int depth = 1; depth <= maxDepth; ++depth)
    {
        std::cout << "info string Searching at depth " << depth << "\n";
        bool timedOut = false;
        SearchResult result = negamaxRoot(board, depth, -MATE_SCORE, MATE_SCORE, start, timeForMove, 0, timedOut);
        Move move = result.bestMove;

        if (!timedOut && std::find(legalMoves.begin(), legalMoves.end(), move) != legalMoves.end())
        {
            bestMove = move;
            std::cout << "info string Best move at depth " << depth << ": " << uci::moveToUci(bestMove) << "\n";
        }
        else if (timedOut)
        {
            std::cout << "info string Search interrupted by time, keeping previous best move\n";
            break;
        }
        else
        {
            std::cout << "info string No legal moves found\n";
            break;
        }

        // Early exit if time is almost up
        double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        if (elapsed > 0.9 * timeForMove)
        {
            std::cout << "info string Stopping iterative deepening due to time\n";
            break;
        }
    }

    return bestMove;
}