#include <iostream>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <bit>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <random>

#include "chess.hpp" // Disservin's library

using namespace chess;

// Material values
static const int MATERIAL_VALUES[6] = {100, 320, 330, 500, 900, 0};
static const int MATE_SCORE = 69000;
static const int MAX_DEPTH = 69;

static const PieceType ptArray[6] = {
    PieceType::PAWN,
    PieceType::KNIGHT,
    PieceType::BISHOP,
    PieceType::ROOK,
    PieceType::QUEEN,
    PieceType::KING};

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



// Transposition table
struct TTEntry
{
    int depth;
    int value;
    Move move;
    enum Flag
    {
        EXACT,
        LOWERBOUND,
        UPPERBOUND
    } flag;
};
std::unordered_map<uint64_t, TTEntry> TT;

// =====================
// Polyglot Opening Book
// =====================
struct PolyglotEntry {
    uint64_t key;
    uint16_t move;
    uint16_t weight;
    uint32_t learn;
};

static std::unordered_multimap<uint64_t, PolyglotEntry> BOOK;
static bool BOOK_LOADED = false;
static std::string BOOK_PATH = "baron30.bin";

static inline uint16_t be16(uint16_t x) { return (uint16_t)((x >> 8) | (x << 8)); }
static inline uint32_t be32(uint32_t x) { return ((x & 0xFF) << 24) | ((x & 0xFF00) << 8) | ((x & 0xFF0000) >> 8) | ((x >> 24) & 0xFF); }
static inline uint64_t be64(uint64_t x) {
    return  ((x & 0x00000000000000FFull) << 56) |
            ((x & 0x000000000000FF00ull) << 40) |
            ((x & 0x0000000000FF0000ull) << 24) |
            ((x & 0x00000000FF000000ull) << 8)  |
            ((x & 0x000000FF00000000ull) >> 8)  |
            ((x & 0x0000FF0000000000ull) >> 24) |
            ((x & 0x00FF000000000000ull) >> 40) |
            ((x & 0xFF00000000000000ull) >> 56);
}

bool loadPolyglotBook(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        std::cout << "info string Could not open book: " << path << "\n";
        return false;
    }
    BOOK.clear();
    while (true) {
        uint64_t k; uint16_t m; uint16_t w; uint32_t l;
        f.read(reinterpret_cast<char*>(&k), 8);
        if (!f) break;
        f.read(reinterpret_cast<char*>(&m), 2);
        f.read(reinterpret_cast<char*>(&w), 2);
        f.read(reinterpret_cast<char*>(&l), 4);
        PolyglotEntry raw;
        raw.key = be64(k);
        raw.move = be16(m);
        raw.weight = be16(w);
        raw.learn = be32(l);
        BOOK.emplace(raw.key, raw);
    }
    BOOK_LOADED = true;
    std::cout << "info string Loaded Polyglot book entries: " << BOOK.size() << " from " << path << "\n";
    return true;
}

std::optional<Move> getBookMove(const Board& board) {
    if (!BOOK_LOADED) {
        loadPolyglotBook(BOOK_PATH);
    }
    uint64_t key = board.hash();
    auto range = BOOK.equal_range(key);
    if (range.first == range.second) return std::nullopt;
    uint64_t total = 0;
    for (auto it = range.first; it != range.second; ++it) total += std::max<uint16_t>(1, it->second.weight);
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist(1, total);
    uint64_t pick = dist(gen);
    const PolyglotEntry* chosen = nullptr;
    for (auto it = range.first; it != range.second; ++it) {
        uint64_t w = std::max<uint16_t>(1, it->second.weight);
        if (pick <= w) { chosen = &it->second; break; }
        pick -= w;
    }
    if (!chosen) return std::nullopt;
    
    Move chosenMove(chosen->move);
    std::string uciMove = uci::moveToUci(chosenMove);

    Movelist legal;
    movegen::legalmoves(legal, board);
    for (auto m : legal) {
        if (uci::moveToUci(m) == uciMove) return m;
    }
    return std::nullopt;
}

// Zobrist key
inline uint64_t boardKey(const Board &board)
{
    return board.hash();
}

// Lookup
std::optional<std::pair<int, Move>> ttLookup(const Board &board, int depth, int alpha, int beta, int plyFromRoot)
{
    auto key = boardKey(board);
    auto it = TT.find(key);
    if (it != TT.end())
    {
        const auto &e = it->second;
        if (e.depth >= depth)
        {
            int val = e.value;
            if (val > MATE_SCORE - 1000)
                val -= plyFromRoot;
            else if (val < -MATE_SCORE + 1000)
                val += plyFromRoot;
            if (e.flag == TTEntry::EXACT)
                return std::make_pair(val, e.move);
            else if (e.flag == TTEntry::LOWERBOUND && val > alpha)
                alpha = val;
            else if (e.flag == TTEntry::UPPERBOUND && val < beta)
                beta = val;
            if (alpha >= beta)
                return std::make_pair(val, e.move);
        }
    }
    return std::nullopt;
}

// Store
void ttStore(const Board &board, int depth, Move move, int value, int alpha, int beta, int plyFromRoot)
{
    TTEntry entry;
    entry.depth = depth;
    // Normalize mate scores with ply distance
    if (value > MATE_SCORE - 1000)
        entry.value = value + plyFromRoot;
    else if (value < -MATE_SCORE + 1000)
        entry.value = value - plyFromRoot;
    else
        entry.value = value;
    entry.move = move;
    if (value <= alpha)
        entry.flag = TTEntry::UPPERBOUND;
    else if (value >= beta)
        entry.flag = TTEntry::LOWERBOUND;
    else
        entry.flag = TTEntry::EXACT;
    TT[boardKey(board)] = entry;
}

// Returns material value of a piece on a square
int getPieceValue(const Board &board, Square sq)
{
    Piece piece = board.at(sq);
    if (piece == Piece::NONE)
        return 0;
    return MATERIAL_VALUES[(int)piece.type()];
}

std::vector<Move> orderMoves(Board &board, Movelist &moves, int plyFromRoot = 0)
{
    std::vector<std::pair<int, Move>> scoredMoves;

    for (auto move : moves)
    {
        int score = 0;

        board.makeMove(move);
        bool isCheck = board.inCheck();
        board.unmakeMove(move);

        if (isCheck)
            score = 200;
        else if (board.isCapture(move))
        {
            int capturedValue = 0;
            if (move.typeOf() == chess::Move::ENPASSANT)
                capturedValue = MATERIAL_VALUES[(int)PieceType::PAWN];
            else
                capturedValue = getPieceValue(board, move.to());
            int capturingValue = getPieceValue(board, move.from());
            score = 100 + ((capturedValue - capturingValue) / 10);
        }
        else if (move.typeOf() == chess::Move::PROMOTION)
            score = 90;
        else if (move.typeOf() == chess::Move::CASTLING)
            score = 80;
        else if (move.typeOf() == chess::Move::ENPASSANT)
            score = 50;

        scoredMoves.push_back({score, move});
    }

    std::sort(scoredMoves.begin(), scoredMoves.end(),
              [](const std::pair<int, Move> &a, const std::pair<int, Move> &b)
              {
                  return a.first > b.first;
              });

    std::vector<Move> ordered;
    for (auto &p : scoredMoves)
        ordered.push_back(p.second);

    return ordered;
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
    if (board.sideToMove() == color)
    {
        movegen::legalmoves(moves, board);
    }
    else
    {
        Board temp = board;
        temp.makeNullMove();
        movegen::legalmoves(moves, temp);
    }
    return moves.size();
}

// Main evaluation function: always returns score from White's perspective (positive = good for White)
int evaluateBoard(const Board &board, int plyFromRoot, Movelist &moves)
{

    if (moves.empty())
    {
        if (board.sideToMove() == Color::WHITE)
            return board.inCheck() ? (-MATE_SCORE +plyFromRoot) : 0;
        else
            return board.inCheck() ? (-MATE_SCORE + plyFromRoot) : 0;
    }

    int score = 0;
    // for (size_t i = 0; i < 6; ++i)
    // {
    //     PieceType pt = ptArray[i];
    //     chess::Bitboard wbb = board.pieces(pt, Color::WHITE);
    //     chess::Bitboard bbb = board.pieces(pt, Color::BLACK);

    //     score += wbb.count() * MATERIAL_VALUES[i];
    //     score -= bbb.count() * MATERIAL_VALUES[i];
    // }

    // Material and PST
    for (Color color : {Color::WHITE, Color::BLACK})
    {
        int colorSign = (color == Color::WHITE) ? 1 : -1;
        for (size_t i = 0; i < 6; ++i)
        {
            PieceType pt = ptArray[i];
            // Count material

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

    // --- Bishop pair bonus ---
    // Give a bonus if a side has two or more bishops
    const int BISHOP_PAIR_BONUS = 40;
    for (Color color : {Color::WHITE, Color::BLACK})
    {
        int count = board.pieces(PieceType::BISHOP, color).count();
        if (count >= 2)
        {
            score += (color == Color::WHITE ? 1 : -1) * BISHOP_PAIR_BONUS;
        }
    }

    // Pawn structure
    score += pawnStructure(board, Color::WHITE);
    score -= pawnStructure(board, Color::BLACK);

    // King safety
    score -= kingSafety(board, Color::WHITE);
    score += kingSafety(board, Color::BLACK);

    // Mobility
    score += (board.sideToMove() == Color::WHITE ? 1 : -1) * (moves.size() * 5); 


    if (board.sideToMove() == Color::BLACK)
        score = -score;

    return score;
}

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

struct SearchResult
{
    int score;
    Move bestMove;
};
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

    if (depth <= 0)
        return quiesce(board, alpha, beta, plyFromRoot+1);

    int bestScore = INT_MIN;
    Move bestMove = Move::NULL_MOVE;
    int originalAlpha = alpha;

    std::vector<Move> orderedMoves = orderMoves(board, legalMoves, plyFromRoot);
    for (auto move : orderedMoves)
    {
        board.makeMove(move);
        int score = -negamax(board, depth - 1, -beta, -alpha, start, timeLimit, plyFromRoot + 1, timedOut);
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
            break;
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

    std::vector<Move> orderedMoves = orderMoves(board, legalMoves, plyFromRoot);
    for (auto move : orderedMoves)
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

Move findBestMoveIterative(Board &board, int maxDepth, double totalTimeRemaining, double increment = 0.0)
{
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

struct Puzzle
{
    std::string fen;
    std::string description;
    std::string expected_best_move;
    int requiredDepth;
};

void runPuzzleTests()
{
    std::vector<Puzzle> puzzles = {
        {"kbK5/pp6/1P6/8/8/8/R7/8 w - - 0 2", "mate in 2 (a2a6)", "a2a6", 4},
        {"rnbqkbnr/ppp2ppp/3p4/4p3/4P1Q1/8/PPPP1PPP/RNB1KBNR b KQkq - 1 3", "black wins a queen (c8g4)", "c8g4", 6},
        {"rnbqkbnr/1pp2ppp/p2p4/4p1B1/4P3/3P4/PPP2PPP/RN1QKBNR w KQkq - 0 4", "white wins a queen (g5d8)", "g5d8", 6},
        {"r1b1kb1r/pppp1ppp/5q2/4n3/3KP3/2N3PN/PPP4P/R1BQ1B1R b kq - 0 1", "", "f8c5", 6},
        {"1r5k/5ppp/3Q4/8/8/Prq3P1/2P1K2P/3R1R2 b - - 5 27", "", "c3e3", 6},
        {"8/1Q6/2PBK3/k7/8/2P2P2/8/7q w - - 7 63", "mate in 2", "d6c7", 4},
        {"r3k2r/ppp2Npp/1b5n/4p2b/2B1P2q/BQP2P2/P5PP/RN5K w kq - 1 0", "mate in 3", "c4b5", 6},
        {"r2n1rk1/1ppb2pp/1p1p4/3Ppq1n/2B3P1/2P4P/PP1N1P1K/R2Q1RN1 b - - 0 1", "mate in 3", "f5f2", 6},
        {"8/8/8/3k4/1Q1Np2p/1p2P2P/1Pp2b2/2K5 w - - 1 50", "mate in 6", "b4a5", 12},

    };

    int passCount = 0;
    int total = (int)puzzles.size();

    auto overall_start = std::chrono::steady_clock::now();

    for (const auto &puzzle : puzzles)
    {
        auto start = std::chrono::steady_clock::now();

        Board board;
        board.setFen(puzzle.fen);
        Move bestMove = findBestMoveIterative(board, puzzle.requiredDepth, 1000);
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
        TT.clear();
    }

    auto overall_end = std::chrono::steady_clock::now();
    double overall_elapsed = std::chrono::duration<double>(overall_end - overall_start).count();

    std::cout << "Puzzle tests passed: " << passCount << " / " << total << std::endl;
    std::cout << "Total time for all puzzles: " << overall_elapsed << "s" << std::endl;
}

bool runSingleTest(const std::string& fen, const std::string& expectedMove, int depth) {
    Board board;
    board.setFen(fen);
    TT.clear();
    
    // Allocate plenty of time for the test
    Move bestMove = findBestMoveIterative(board, depth, 60.0);
    std::string bestMoveUci = uci::moveToUci(bestMove);
    
    bool passed = (bestMoveUci == expectedMove);
    
    if (passed) {
        std::cout << "[PASS] Found best move: " << bestMoveUci << std::endl;
    } else {
        std::cout << "[FAIL] Expected: " << expectedMove << ", Got: " << bestMoveUci << std::endl;
    }
    
    return passed;
}

// Main loop (UCI)
int main(int argc, char* argv[])
{

    // Check for test mode
    if (argc > 1 && std::string(argv[1]) == "--test") {
        if (argc < 5) {
            std::cerr << "Usage: " << argv[0] << " --test [FEN] [expected_move] [depth]" << std::endl;
            return 1;
        }
        std::string fen = argv[2];
        std::string expectedMove = argv[3];
        int depth = std::stoi(argv[4]);
        
        bool result = runSingleTest(fen, expectedMove, depth);
        return result ? 0 : 1;  // Return success (0) only if test passes
    }

    Board board;

    // board.setFen(chess::constants::STARTPOS);
    // std::cout << "starting position hash: " << board.hash() << std::endl;

    // board.setFen("r1bqkbnr/pppp1ppp/3np3/8/3PPB2/2N2N2/PP3PPP/R2QKB1R b KQkq - 1 6");
    // // board.setFen("r1bqkb1r/pppp1ppp/3npn2/8/3PPB2/2N2N2/PP3PPP/R2QKB1R w KQkq - 2 7)");
    // board.setFen("8/8/8/3k4/1Q1Np2p/1p2P2P/1Pp2b2/2K5 w - - 1 50");

    // // runPuzzleTests();
    // double timeLimit = 300.0; // seconds
    // int maxDepth = 13;
    // bool timedOut = false;
    // auto start = std::chrono::steady_clock::now();

    // Move best = findBestMoveIterative(board, maxDepth, timeLimit, timedOut);
    // std::cout << "Best move: " << uci::moveToUci(best) << std::endl;

    // return 0;

    std::string line;
    // Uncomment below line to run puzzle tests before starting UCI loop

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
            double total_time_remaining = 5.0; // default seconds
            double increment = 0.0;
            int moveNumber = board.fullMoveNumber();

            std::istringstream ss(line);
            std::string token;
            while (ss >> token)
            {
                // if (token == "depth")
                // {
                //     ss >> MAX_DEPTH;
                // }
                if (token == "movetime")
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
                else if (token == "winc" && board.sideToMove() == chess::Color::WHITE)
                {
                    int ms;
                    ss >> ms;
                    increment = ms / 1000.0;
                }
                else if (token == "binc" && board.sideToMove() == chess::Color::BLACK)
                {
                    int ms;
                    ss >> ms;
                    increment = ms / 1000.0;
                }
            }

            // Try Polyglot book first
            if (BOOK_LOADED || loadPolyglotBook(BOOK_PATH)) {
                if (auto bm = getBookMove(board)) {
                    std::cout << "info string book move found\n";
                    std::cout << "bestmove " << uci::moveToUci(*bm) << "\n";
                    continue;
                }
            }

            Move best = findBestMoveIterative(board, MAX_DEPTH, total_time_remaining, increment);
            std::cout << "bestmove " << uci::moveToUci(best) << "\n";
        }
        else if (line == "quit")
        {
            break;
        }
        else if (line == "puzzletest")
        {
            runPuzzleTests();
            std::cout << "info string Puzzle tests complete\n";
        }
    }
}