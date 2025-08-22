#include "utils.hpp"
#include "eval.hpp"

using namespace chess;

// utils.cpp
#include "utils.hpp"

int mirror(int idx) {
    return ((7 - (idx / 8)) * 8) + (idx % 8);
}

int countBits(chess::Bitboard bb) {
#if __cpp_lib_bitops >= 201907L
    return std::popcount(bb.getBits());
#else
    return __builtin_popcountll(bb.getBits());
#endif
}

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

std::vector<Move> orderMoves(Board &board, Movelist &moves, int plyFromRoot)
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