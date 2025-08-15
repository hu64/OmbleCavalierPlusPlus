#include <iostream>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <bit>
#include <algorithm>
#include <sstream>

#include "chess.hpp" // Disservin's library

using namespace chess;

// Material values
static const int MATERIAL_VALUES[6] = {100, 320, 330, 500, 900, 60000};

// Piece-square tables (values for white, black uses mirrored)
static const int PAWN_PST[64] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
    5, 5, 10, 25, 25, 10, 5, 5,
    0, 0, 0, 20, 20, 0, 0, 0,
    5, -5, -10, 0, 0, -10, -5, 5,
    5, 10, 10, -20, -20, 10, 10, 5,
    0, 0, 0, 0, 0, 0, 0, 0};
static const int KNIGHT_PST[64] = {
    -50, -40, -30, -30, -30, -30, -40, -50,
    -40, -20, 0, 0, 0, 0, -20, -40,
    -30, 0, 10, 15, 15, 10, 0, -30,
    -30, 5, 15, 20, 20, 15, 5, -30,
    -30, 0, 15, 20, 20, 15, 0, -30,
    -30, 5, 10, 15, 15, 10, 5, -30,
    -40, -20, 0, 5, 5, 0, -20, -40,
    -50, -40, -30, -30, -30, -30, -40, -50};
static const int BISHOP_PST[64] = {
    -20, -10, -10, -10, -10, -10, -10, -20,
    -10, 0, 0, 0, 0, 0, 0, -10,
    -10, 0, 5, 10, 10, 5, 0, -10,
    -10, 5, 5, 10, 10, 5, 5, -10,
    -10, 0, 10, 10, 10, 10, 0, -10,
    -10, 10, 10, 10, 10, 10, 10, -10,
    -10, 5, 0, 0, 0, 0, 5, -10,
    -20, -10, -10, -10, -10, -10, -10, -20};
static const int ROOK_PST[64] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    5, 10, 10, 10, 10, 10, 10, 5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    0, 0, 0, 5, 5, 0, 0, 0};
static const int QUEEN_PST[64] = {
    -20, -10, -10, -5, -5, -10, -10, -20,
    -10, 0, 0, 0, 0, 0, 0, -10,
    -10, 0, 5, 5, 5, 5, 0, -10,
    -5, 0, 5, 5, 5, 5, 0, -5,
    0, 0, 5, 5, 5, 5, 0, -5,
    -10, 5, 5, 5, 5, 5, 0, -10,
    -10, 0, 5, 0, 0, 0, 0, -10,
    -20, -10, -10, -5, -5, -10, -10, -20};
static const int KING_PST[64] = {
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -20, -30, -30, -40, -40, -30, -30, -20,
    -10, -20, -20, -20, -20, -20, -20, -10,
    20, 20, 0, 0, 0, 0, 20, 20,
    20, 30, 10, 0, 0, 10, 30, 20};

int pstValue(Piece piece, Square sq)
{
    if (piece == Piece::NONE)
        return 0;
    int idx = sq.index();
    // Mirror for black
    if (piece.color() == Color::BLACK)
        idx = 56 + (7 - (idx % 8)) - 8 * (idx / 8);
    switch (piece.type())
    {
    case (int)PieceType::PAWN:
        return PAWN_PST[idx];
    case (int)PieceType::KNIGHT:
        return KNIGHT_PST[idx];
    case (int)PieceType::BISHOP:
        return BISHOP_PST[idx];
    case (int)PieceType::ROOK:
        return ROOK_PST[idx];
    case (int)PieceType::QUEEN:
        return QUEEN_PST[idx];
    case (int)PieceType::KING:
        return KING_PST[idx];
    default:
        return 0;
    }
}

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
            int capturedValue = 0;
            // Handle en passant correctly: destination square is empty pre-move
            if (move.typeOf() == chess::Move::ENPASSANT)
                capturedValue = MATERIAL_VALUES[(int)PieceType::PAWN];
            else
                capturedValue = getPieceValue(board, move.to());
            int capturingValue = getPieceValue(board, move.from());
            // MVV-LVA style capture ordering
            score = 100 + ((capturedValue - capturingValue) / 10);
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

// Helper: mirror square for black
inline int mirror(int idx) { return ((7 - (idx / 8)) * 8) + (idx % 8); }

// Fast bit count for pawn structure
inline int countBits(chess::Bitboard bb)
{
#if __cpp_lib_bitops >= 201907L
    return std::popcount(bb.getBits());
#else
    return __builtin_popcountll(bb.getBits());
#endif
}

// King safety: penalty for open files and missing pawn shield
int kingSafety(const Board &board, Color color)
{
    int penalty = 0;
    Square kingSq = board.kingSq(color);
    int kfile = kingSq.file();
    int krank = kingSq.rank();

    // Pawn shield squares in front of king (3 squares)
    int shieldRank = (color == Color::WHITE) ? krank + 1 : krank - 1;
    for (int df = -1; df <= 1; ++df)
    {
        int f = kfile + df;
        if (f < 0 || f > 7 || shieldRank < 0 || shieldRank > 7)
            continue;
        Square sq = Square(f + shieldRank * 8);
        Piece p = board.at(sq);
        if (p.type() != PieceType::PAWN || p.color() != color)
            penalty += 15; // missing pawn shield
    }

    // Penalty for open/semi-open files near king
    for (int df = -1; df <= 1; ++df)
    {
        int f = kfile + df;
        if (f < 0 || f > 7)
            continue;
        chess::Bitboard pawns = board.pieces(PieceType::PAWN, color) & chess::Bitboard(File(f));
        chess::Bitboard oppPawns = board.pieces(PieceType::PAWN, ~color) & chess::Bitboard(File(f));
        if (!pawns)
        {
            penalty += oppPawns ? 10 : 20; // semi-open or open file
        }
    }
    return penalty;
}

// Pawn structure: doubled, isolated, passed pawns
int pawnStructure(const Board &board, Color color)
{
    int penalty = 0, bonus = 0;
    chess::Bitboard pawns = board.pieces(PieceType::PAWN, color);

    // Doubled pawns
    for (int f = 0; f < 8; ++f)
    {
        chess::Bitboard filePawns = pawns & chess::Bitboard(File(f));
        int count = countBits(filePawns);
        if (count > 1)
            penalty += 12 * (count - 1);
    }

    // Isolated pawns
    for (int f = 0; f < 8; ++f)
    {
        chess::Bitboard filePawns = pawns & chess::Bitboard(File(f));
        if (!filePawns)
            continue;
        bool hasLeft = (f > 0) && (pawns & chess::Bitboard(File(f - 1)));
        bool hasRight = (f < 7) && (pawns & chess::Bitboard(File(f + 1)));
        if (!hasLeft && !hasRight)
            penalty += 15 * countBits(filePawns);
    }

    // Passed pawns
    chess::Bitboard oppPawns = board.pieces(PieceType::PAWN, ~color);
    for (int sq = 0; sq < 64; ++sq)
    {
        if (!pawns.check(sq))
            continue;
        int file = sq % 8;
        int rank = sq / 8;
        bool isPassed = true;
        for (int df = -1; df <= 1; ++df)
        {
            int f = file + df;
            if (f < 0 || f > 7)
                continue;
            for (int r = (color == Color::WHITE ? rank + 1 : 0);
                 (color == Color::WHITE ? r < 8 : r < rank);
                 r += (color == Color::WHITE ? 1 : 1))
            {
                int idx = f + r * 8;
                if (oppPawns.check(idx))
                    isPassed = false;
            }
        }
        if (isPassed)
            bonus += 20;
    }
    return bonus - penalty;
}

// Mobility: number of legal moves for each side
int mobility(const Board &board, Color color)
{
    chess::Movelist moves;
    Board copy = board;
    if (copy.sideToMove() != color)
        copy.makeNullMove();
    movegen::legalmoves(moves, copy);
    return moves.size();
}

// Main evaluation function
int evaluateBoard(const Board &board, int plyFromRoot)
{
    if (board.isHalfMoveDraw() || board.isRepetition())
        return 0;

    chess::Movelist legalMoves;
    movegen::legalmoves(legalMoves, board);

    if (legalMoves.empty())
        return board.inCheck() ? -(100000 - plyFromRoot) : 0;

    int score = 0;

    // for (PieceType pt : {PieceType::PAWN, PieceType::KNIGHT, PieceType::BISHOP,
    //                      PieceType::ROOK, PieceType::QUEEN, PieceType::KING})
    // {
    //     chess::Bitboard wbb = board.pieces(pt, Color::WHITE);
    //     chess::Bitboard bbb = board.pieces(pt, Color::BLACK);
    //     score += (int)wbb.count() * MATERIAL_VALUES[(int)pt];
    //     score -= (int)bbb.count() * MATERIAL_VALUES[(int)pt];
    // }

    // Material and PST
    for (Color color : {Color::WHITE, Color::BLACK})
    {
        int colorSign = (color == Color::WHITE) ? 1 : -1;
        for (PieceType pt : {PieceType::PAWN, PieceType::KNIGHT, PieceType::BISHOP,
                             PieceType::ROOK, PieceType::QUEEN, PieceType::KING})
        {
            chess::Bitboard bb = board.pieces(pt, color);
            int pieceValue = MATERIAL_VALUES[static_cast<int>(pt)];
            while (bb)
            {
                int sq = bb.lsb();
                bb.clear(sq);
                score += colorSign * pieceValue;
                // PST: mirror for black
                int pstIdx = (color == Color::WHITE) ? sq : mirror(sq);
                switch (pt)
                {
                case (int)PieceType::PAWN:
                    score += colorSign * PAWN_PST[pstIdx];
                    break;
                case (int)PieceType::KNIGHT:
                    score += colorSign * KNIGHT_PST[pstIdx];
                    break;
                case (int)PieceType::BISHOP:
                    score += colorSign * BISHOP_PST[pstIdx];
                    break;
                case (int)PieceType::ROOK:
                    score += colorSign * ROOK_PST[pstIdx];
                    break;
                case (int)PieceType::QUEEN:
                    score += colorSign * QUEEN_PST[pstIdx];
                    break;
                case (int)PieceType::KING:
                    score += colorSign * KING_PST[pstIdx];
                    break;
                default:
                    break;
                }
            }
        }
    }

    // Pawn structure
    score += pawnStructure(board, Color::WHITE);
    score -= pawnStructure(board, Color::BLACK);

    // King safety
    score -= kingSafety(board, Color::WHITE);
    score += kingSafety(board, Color::BLACK);

    // Mobility
    score += 5 * (mobility(board, Color::WHITE) - mobility(board, Color::BLACK));

    // Side to move bonus
    if (board.sideToMove() == Color::WHITE)
        score += 10;
    else
        score -= 10;
    if (board.sideToMove() == Color::BLACK)
        score = -score;

    return score;
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
            continue; // Only captures in quiescence

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

    auto ttVal = ttLookup(board, depth, alpha, beta);
    if (ttVal.has_value())
        return ttVal.value();

    chess::Movelist moves;
    movegen::legalmoves(moves, board);
    if (depth <= 0)
        return quiesce(board, alpha, beta, plyFromRoot);

    int originalAlpha = alpha;
    int bestScore = -1000000;

    std::vector<Move> orderedMoves = orderMoves(board, moves);
    for (auto move : orderedMoves)
    {
        board.makeMove(move);
        int score = -negamax(board, depth - 1, -beta, -alpha, start, timeLimit, plyFromRoot + 1, timedOut);
        board.unmakeMove(move);

        if (timedOut)
            return alpha; // abort search early on timeout

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
        if (timedOut)
            break;
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

struct Puzzle
{
    std::string fen;
    std::string description;
    std::string expected_best_move;
};

void runPuzzleTests()
{
    std::vector<Puzzle> puzzles = {
        {"kbK5/pp6/1P6/8/8/8/R7/8 w - - 0 2", "mate in 2 (a2a6)", "a2a6"},
        {"rnbqkbnr/ppp2ppp/3p4/4p3/4P1Q1/8/PPPP1PPP/RNB1KBNR b KQkq - 1 3", "black wins a queen (c8g4)", "c8g4"},
        {"rnbqkbnr/1pp2ppp/p2p4/4p1B1/4P3/3P4/PPP2PPP/RN1QKBNR w KQkq - 0 4", "white wins a queen (g5d8)", "g5d8"},
        {"r1b1kb1r/pppp1ppp/5q2/4n3/3KP3/2N3PN/PPP4P/R1BQ1B1R b kq - 0 1", "", "f8c5"}};

    int passCount = 0;
    int total = (int)puzzles.size();

    auto overall_start = std::chrono::steady_clock::now();

    for (const auto &puzzle : puzzles)
    {
        auto start = std::chrono::steady_clock::now();

        Board board;
        board.setFen(puzzle.fen);
        Move bestMove = findBestMoveIterative(board, 6, 1000);
        std::string bestMoveUci = uci::moveToUci(bestMove);

        auto end = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(end - start).count();

        bool passed = (bestMoveUci == puzzle.expected_best_move);
        if (passed)
        {
            std::cout << "[PASS] ";
            ++passCount;
        }
        else
        {
            std::cout << "[FAIL] ";
        }
        std::cout << "FEN: " << puzzle.fen;
        if (!puzzle.description.empty())
        {
            std::cout << " (" << puzzle.description << ")";
        }
        std::cout << " - Expected: " << puzzle.expected_best_move << ", Got: " << bestMoveUci;
        std::cout << " | Time: " << elapsed << "s" << std::endl;
    }

    auto overall_end = std::chrono::steady_clock::now();
    double overall_elapsed = std::chrono::duration<double>(overall_end - overall_start).count();

    std::cout << "Puzzle tests passed: " << passCount << " / " << total << std::endl;
    std::cout << "Total time for all puzzles: " << overall_elapsed << "s" << std::endl;
}

// Main loop (UCI)
int main()
{
    Board board;
    std::string line;
    int depth = 30;

    // Uncomment below line to run puzzle tests before starting UCI loop
    // runPuzzleTests();
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