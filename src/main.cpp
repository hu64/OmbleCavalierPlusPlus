#include <iostream>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <bit>

#include "chess.hpp" // Disservin's library

using namespace chess;

// Material values
static const int MATERIAL_VALUES[6] = {100, 320, 330, 500, 900, 60000};

// Returns material value of a piece on a square
int getPieceValue(const Board &board, Square sq)
{
    Piece piece = board.at(sq);
    if (piece == Piece::NONE)
        return 0;
    return MATERIAL_VALUES[(int)piece.type()];
}

// Order moves by heuristic: captures, checks, promotions
std::vector<Move> orderMoves(Board &board, Movelist &moves)
{
    // Pair of <score, move>
    std::vector<std::pair<int, Move>> scoredMoves;

    for (auto move : moves)
    {
        int score = 0;

        board.makeMove(move);
        bool isCheck = board.inCheck();
        board.unmakeMove(move);

        if (isCheck)
        {
            score = 200;
        }
        else if (board.isCapture(move))
        {
            int capturedValue = getPieceValue(board, move.to());
            int capturingValue = getPieceValue(board, move.from());
            score = 100 + ((capturedValue - capturingValue) / 100);
        }
        else if (move.typeOf() == chess::Move::PROMOTION)
        {
            score = 90;
        }
        else if (move.typeOf() == chess::Move::CASTLING)
        {
            score = 80;
        }
        else if (move.typeOf() == chess::Move::ENPASSANT)
        {
            score = 50;
        }
        scoredMoves.push_back({score, move});
    }

    // Sort moves by score descending
    std::sort(scoredMoves.begin(), scoredMoves.end(),
              [](const std::pair<int, Move> &a, const std::pair<int, Move> &b)
              {
                  return a.first > b.first;
              });

    // Extract sorted moves
    std::vector<Move> ordered;
    for (auto &p : scoredMoves)
        ordered.push_back(p.second);

    return ordered;
}

// Transposition table
struct TTEntry
{
    int depth;
    int value;
    enum Flag
    {
        EXACT,
        LOWERBOUND,
        UPPERBOUND
    } flag;
};
std::unordered_map<uint64_t, TTEntry> TT;

// Zobrist key
inline uint64_t boardKey(const Board &board)
{
    return board.hash();
}

// Lookup
std::optional<int> ttLookup(const Board &board, int depth, int alpha, int beta)
{
    auto key = boardKey(board);
    auto it = TT.find(key);
    if (it != TT.end())
    {
        const auto &e = it->second;
        if (e.depth >= depth)
        {
            if (e.flag == TTEntry::EXACT)
                return e.value;
            else if (e.flag == TTEntry::LOWERBOUND && e.value > alpha)
                alpha = e.value;
            else if (e.flag == TTEntry::UPPERBOUND && e.value < beta)
                beta = e.value;
            if (alpha >= beta)
                return e.value;
        }
    }
    return std::nullopt;
}

// Store
void ttStore(const Board &board, int depth, int value, int alpha, int beta)
{
    TTEntry entry;
    entry.depth = depth;
    entry.value = value;
    if (value <= alpha)
        entry.flag = TTEntry::UPPERBOUND;
    else if (value >= beta)
        entry.flag = TTEntry::LOWERBOUND;
    else
        entry.flag = TTEntry::EXACT;
    TT[boardKey(board)] = entry;
}

// Evaluation
int evaluateBoard(const Board &board, int plyFromRoot, Movelist &moves)
{

    if (board.isHalfMoveDraw())
        return board.getHalfMoveDrawType().first == GameResultReason::CHECKMATE ? -(100000 - plyFromRoot) : 0;

    if (board.isRepetition())
        return 0;

    bool inCheck = board.inCheck();
    if (moves.empty())
        return inCheck ? -(100000 - plyFromRoot) : 0;

    int materialScore = 0;

    for (PieceType pt : {PieceType::PAWN, PieceType::KNIGHT, PieceType::BISHOP,
                         PieceType::ROOK, PieceType::QUEEN, PieceType::KING})
    {
        chess::Bitboard wbb = board.pieces(pt, Color::WHITE);
        chess::Bitboard bbb = board.pieces(pt, Color::BLACK);

        materialScore += wbb.count() * MATERIAL_VALUES[(int)pt];
        materialScore -= bbb.count() * MATERIAL_VALUES[(int)pt];
    }
    int score = materialScore * (board.sideToMove() == Color::WHITE ? 1 : -1);

    // Mobility bonus
    int mobilityBonus = 10 * moves.size();
    score += mobilityBonus * (board.sideToMove() == Color::WHITE ? 1 : -1);
    score += mobilityBonus;

    return score;
}

// Quiescence search
int quiesce(Board &board, int alpha, int beta, int plyFromRoot, Movelist &moves)
{
    int stand_pat = evaluateBoard(board, plyFromRoot, moves);
    if (stand_pat >= beta)
        return beta;
    if (stand_pat > alpha)
        alpha = stand_pat;


    for (auto move : moves)
    {
        if (!board.isCapture(move))
            continue;
        board.makeMove(move);
        chess::Movelist nextMoves;
        movegen::legalmoves(nextMoves, board);
        int score = -quiesce(board, -beta, -alpha, plyFromRoot + 1, nextMoves);
        board.unmakeMove(move);
        if (score >= beta)
            return beta;
        if (score > alpha)
            alpha = score;
    }
    return alpha;
}

int negamax(Board &board, int depth, int alpha, int beta,
            std::chrono::steady_clock::time_point start, double timeLimit, int plyFromRoot, bool &timedOut)
{
    using namespace std::chrono;
    if (duration<double>(steady_clock::now() - start).count() > timeLimit)
    {
        timedOut = true;
        return 0;
    }

    auto ttVal = ttLookup(board, depth, alpha, beta);
    if (ttVal.has_value())
        return ttVal.value();

    chess::Movelist moves;
    movegen::legalmoves(moves, board);
    if (depth <= 0)
        return quiesce(board, alpha, beta, plyFromRoot, moves);

    int originalAlpha = alpha;
    int bestScore = -1000000;

    std::vector<Move> orderedMoves = orderMoves(board, moves);
    for (auto move : orderedMoves)
    {
        board.makeMove(move);
        int score = -negamax(board, depth - 1, -beta, -alpha, start, timeLimit, plyFromRoot + 1, timedOut);
        board.unmakeMove(move);

        if (score > bestScore)
            bestScore = score;
        if (score > alpha)
            alpha = score;
        if (alpha >= beta)
            break;
    }

    ttStore(board, depth, bestScore, originalAlpha, beta);
    return bestScore;
}

Move findBestMove(Board &board, int depth,
                  std::chrono::steady_clock::time_point start, double timeLimit,
                  bool &timedOut)
{
    Move bestMove = Move::NULL_MOVE;
    int bestScore = -88888;
    int alpha = -88888;
    int beta = 88888;

    chess::Movelist moves;
    movegen::legalmoves(moves, board);
    std::vector<Move> orderedMoves = orderMoves(board, moves);
    for (auto move : orderedMoves)
    {
        double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        if (elapsed > timeLimit)
        {
            std::cout << "info string Time limit reached in find best move, stopping search\n";
            timedOut = true;
            break;
        }

        board.makeMove(move);
        int score = -negamax(board, depth - 1, -beta, -alpha, start, timeLimit, 1, timedOut);
        board.unmakeMove(move);
        if (score > bestScore)
        {
            bestScore = score;
            bestMove = move;
        }

        alpha = std::max(alpha, score);
        std::cout << "info score cp " << score << " pv " << uci::moveToUci(move) << "\n";
    }

    return bestMove;
}

Move findBestMoveIterative(Board &board, int maxDepth, double totalTimeRemaining)
{
    int moveNumber = board.fullMoveNumber();
    double timeLimit = totalTimeRemaining / std::max(10, 40 - moveNumber);
    auto start = std::chrono::steady_clock::now();

    chess::Movelist legalMoves;
    movegen::legalmoves(legalMoves, board);
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
        Move move = findBestMove(board, depth, start, timeLimit, timedOut);

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
    }

    return bestMove;
}

// Main loop (UCI)
int main()
{
    Board board;
    std::string line;
    int depth = 30;

    while (std::getline(std::cin, line))
    {
        if (line == "uci")
        {
            std::cout << "id name OmbleCavalierCPP\n";
            std::cout << "id author Hughes Perreault\n";
            std::cout << "uciok\n";
        }
        else if (line == "isready")
        {
            std::cout << "readyok\n";
        }
        else if (line == "ucinewgame")
        {
            board.setFen(chess::constants::STARTPOS);
            TT.clear();
        }
        else if (line.rfind("position", 0) == 0)
        {
            if (line.find("startpos") != std::string::npos)
            {
                board.setFen(chess::constants::STARTPOS);
                auto movesPos = line.find("moves");
                if (movesPos != std::string::npos)
                {
                    std::istringstream ss(line.substr(movesPos + 6));
                    std::string moveStr;
                    while (ss >> moveStr)
                    {
                        Move m = uci::uciToMove(board, moveStr);
                        board.makeMove(m);
                    }
                }
            }
        }
        else if (line.rfind("go", 0) == 0)
        {
            double total_time_remaining = 5.0;       // default seconds
            int moveNumber = board.fullMoveNumber(); // depends on your Board API
            int searchDepth = depth;

            std::istringstream ss(line);
            std::string token;
            while (ss >> token)
            {
                if (token == "depth")
                {
                    ss >> searchDepth;
                }
                else if (token == "movetime")
                {
                    int ms;
                    ss >> ms;
                    total_time_remaining = ms / 1000.0;
                }
                else if (token == "wtime" && board.sideToMove() == chess::Color::WHITE)
                {
                    int ms;
                    ss >> ms;
                    total_time_remaining = ms / 1000.0;
                }
                else if (token == "btime" && board.sideToMove() == chess::Color::BLACK)
                {
                    int ms;
                    ss >> ms;
                    total_time_remaining = ms / 1000.0;
                }
            }

            Move best = findBestMoveIterative(board, searchDepth, total_time_remaining);
            std::cout << "bestmove " << uci::moveToUci(best) << "\n";
        }
        else if (line == "quit")
        {
            break;
        }
    }
}