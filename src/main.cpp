#include <iostream>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <bit>

#include "chess.hpp" // Disservin's library

using namespace chess;

// Material values
static const int MATERIAL_VALUES[6] = {100, 320, 330, 500, 900, 60000};

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
int evaluateBoard(const Board &board, int plyFromRoot = 0)
{
    if (board.isHalfMoveDraw())
        return board.getHalfMoveDrawType().first == GameResultReason::CHECKMATE ? -(100000 - plyFromRoot) : 0;

    if (board.isRepetition())
        return 0;

    int score = 0;

    for (PieceType pt : {PieceType::PAWN, PieceType::KNIGHT, PieceType::BISHOP,
                         PieceType::ROOK, PieceType::QUEEN, PieceType::KING})
    {
        chess::Bitboard wbb = board.pieces(pt, Color::WHITE);
        chess::Bitboard bbb = board.pieces(pt, Color::BLACK);

        score += wbb.count() * MATERIAL_VALUES[(int)pt];
        score -= bbb.count() * MATERIAL_VALUES[(int)pt];
    }

    // Mobility bonus
    chess::Movelist moves;
    movegen::legalmoves(moves, board);
    score += 10 * moves.size();

    return board.sideToMove() == Color::WHITE ? score : -score;
}

// Quiescence search
int quiesce(Board &board, int alpha, int beta, int plyFromRoot)
{
    int stand_pat = evaluateBoard(board, plyFromRoot);
    if (stand_pat >= beta)
        return beta;
    if (stand_pat > alpha)
        alpha = stand_pat;

    chess::Movelist moves;
    movegen::legalmoves(moves, board);
    for (auto move : moves)
    {
        if (!board.isCapture(move))
            continue;
        board.makeMove(move);
        int score = -quiesce(board, -beta, -alpha, plyFromRoot + 1);
        board.unmakeMove(move);
        if (score >= beta)
            return beta;
        if (score > alpha)
            alpha = score;
    }
    return alpha;
}

// Negamax
int negamax(Board &board, int depth, int alpha, int beta,
            std::chrono::steady_clock::time_point start, double timeLimit, int plyFromRoot)
{
    using namespace std::chrono;
    if (duration<double>(steady_clock::now() - start).count() > timeLimit)
        return 0; // timeout

    if (board.isHalfMoveDraw())
        return board.getHalfMoveDrawType().first == GameResultReason::CHECKMATE ? -(100000 - plyFromRoot) : 0;

    if (board.isRepetition())
        return 0;


    auto ttVal = ttLookup(board, depth, alpha, beta);
    if (ttVal.has_value())
        return ttVal.value();

    if (depth <= 0)
        return quiesce(board, alpha, beta, plyFromRoot);

    int originalAlpha = alpha;
    int bestScore = -1000000;

    chess::Movelist moves;
    movegen::legalmoves(moves, board);

    for (auto move : moves)
    {
        board.makeMove(move);
        int score = -negamax(board, depth - 1, -beta, -alpha, start, timeLimit, plyFromRoot + 1);
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

// Iterative deepening
Move findBestMoveIterative(Board &board, int maxDepth, double timeLimit)
{
    Move bestMove = Move::NULL_MOVE;
    auto start = std::chrono::steady_clock::now();

    chess::Movelist moves;
    movegen::legalmoves(moves, board);
    if (moves.empty())
        return bestMove;

    for (int depth = 1; depth <= maxDepth; ++depth)
    {
        int bestScore = -1000000;
        for (auto move : moves)
        {
            board.makeMove(move);
            int score = -negamax(board, depth - 1, -1000000, 1000000, start, timeLimit, 1);
            board.unmakeMove(move);
            if (score > bestScore)
            {
                bestScore = score;
                bestMove = move;
            }
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
            Move best = findBestMoveIterative(board, depth, 5.0);
            std::cout << "bestmove " << uci::moveToUci(best) << "\n";
        }
        else if (line == "quit")
        {
            break;
        }
    }
}