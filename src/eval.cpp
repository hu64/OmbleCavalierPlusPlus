#include "eval.hpp"
#include "utils.hpp"
using namespace chess;

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

// Main evaluation function: always returns score from White's perspective (positive = good for White)
int evaluateBoard(const Board &board, int plyFromRoot, Movelist &moves)
{

    if (moves.empty())
    {
        if (board.sideToMove() == Color::WHITE)
            return board.inCheck() ? (-MATE_SCORE + plyFromRoot) : 0;
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
    // Fast material and PST evaluation
    for (Color color : {Color::WHITE, Color::BLACK})
    {
        int colorSign = (color == Color::WHITE) ? 1 : -1;
        for (size_t i = 0; i < 6; ++i)
        {
            PieceType pt = ptArray[i];
            chess::Bitboard bb = board.pieces(pt, color);
            int pieceValue = MATERIAL_VALUES[static_cast<int>(pt)];
            const int* pst = nullptr;
            switch (static_cast<int>(pt))
            {
            case static_cast<int>(PieceType::PAWN):   pst = PAWN_PST; break;
            case static_cast<int>(PieceType::KNIGHT): pst = KNIGHT_PST; break;
            case static_cast<int>(PieceType::BISHOP): pst = BISHOP_PST; break;
            case static_cast<int>(PieceType::ROOK):   pst = ROOK_PST; break;
            case static_cast<int>(PieceType::QUEEN):  pst = QUEEN_PST; break;
            case static_cast<int>(PieceType::KING):   pst = KING_PST; break;
            default: break;
            }
            while (bb)
            {
                int sq = bb.lsb();
                bb.clear(sq);
                score += colorSign * pieceValue;
                if (pst)
                {
                    int pstIdx = (color == Color::WHITE) ? sq : mirror(sq);
                    score += colorSign * pst[pstIdx];
                }
            }
        }
    }

    // --- Bishop pair bonus ---
    // Give a bonus if a side has two or more bishops
    const int BISHOP_PAIR_BONUS = 30;
    for (Color color : {Color::WHITE, Color::BLACK})
    {
        int count = board.pieces(PieceType::BISHOP, color).count();
        if (count >= 2)
        {
            score += (color == Color::WHITE ? 1 : -1) * BISHOP_PAIR_BONUS;
        }
    }

    // Pawn structure
    // score += pawnStructure(board, Color::WHITE);
    // score -= pawnStructure(board, Color::BLACK);

    // King safety
    score -= kingSafety(board, Color::WHITE);
    score += kingSafety(board, Color::BLACK);

    // Mobility
    score += (board.sideToMove() == Color::WHITE ? 1 : -1) * (moves.size() * 5);

    if (board.sideToMove() == Color::BLACK)
        score = -score;

    return score;
}
