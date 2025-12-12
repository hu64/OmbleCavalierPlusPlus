#include "utils.hpp"
#include "eval.hpp"
#include <algorithm>
#include <optional>
#include <vector>

using namespace chess;

int mirror(int idx)
{
    return ((7 - (idx / 8)) * 8) + (idx % 8);
}

int countBits(chess::Bitboard bb)
{
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

// MVV-LVA scoring for captures
int mvvLvaScore(const Board &board, const Move &move)
{
    if (!board.isCapture(move))
        return 0;
    int victim = getPieceValue(board, move.to());
    int attacker = getPieceValue(board, move.from());
    return 10 * victim - attacker;
}

// Simple Static Exchange Evaluation (greedy recapture sequence on a board copy)
int staticExchangeEvaluation(Board board, Move move)
{
    using namespace chess;
    const int INF = 1000000;
    Square sq = move.to();
    Color us = board.sideToMove();

    // initial captured value (0 if no capture)
    int captured = getPieceValue(board, sq);

    // make the initial capture on a copy
    board.makeMove(move);

    std::vector<int> gains;
    gains.push_back(captured);

    Color stm = board.sideToMove();
    // loop capturing the victim greedily by least-value attackers
    while (true)
    {
        auto attackers = attacks::attackers(board, stm, sq);
        if (!attackers)
            break;

        // find least valuable attacker
        int fromSq = -1;
        int bestVal = INF;
        for (auto pt : {PieceType::PAWN, PieceType::KNIGHT, PieceType::BISHOP, PieceType::ROOK, PieceType::QUEEN, PieceType::KING})
        {
            auto bb = board.pieces(pt, stm) & attackers;
            if (bb)
            {
                int sqi = bb.lsb();
                int val = MATERIAL_VALUES[static_cast<int>(pt)];
                if (val < bestVal)
                {
                    bestVal = val;
                    fromSq = sqi;
                }
            }
        }
        if (fromSq < 0)
            break;

        // make the capture (from -> to)
        Square fromSqSquare(fromSq);
        Move cap = Move::make<Move::NORMAL>(fromSqSquare, sq);
        board.makeMove(cap);

        // the value gained on this ply is the value of the piece that was captured
        gains.push_back(bestVal);
        stm = board.sideToMove();
    }

    // minimax the capture sequence values
    for (int i = (int)gains.size() - 2; i >= 0; --i)
    {
        gains[i] = std::max(-gains[i + 1], gains[i]);
    }

    return gains.empty() ? 0 : gains[0];
}

// Move ordering: hash move > captures (MVV-LVA) > killer moves > history > quiets
std::vector<Move> orderMoves(
    Board &board, Movelist &moves, int plyFromRoot,
    const std::optional<Move> &hashMove,
    const std::vector<Move> &killerMoves,
    long long historyHeuristic[64][64])
{
    std::vector<std::pair<int, Move>> scoredMoves;

    for (auto move : moves)
    {
        int score = 0;

        // 1. Hash move
        if (hashMove && move == *hashMove)
        {
            score = 1000000;
        }
        // 2. Captures (MVV-LVA)
        else if (board.isCapture(move))
        {
            // penalize obviously losing captures using SEE
            int seeVal = staticExchangeEvaluation(board, move);
            if (seeVal < 0)
                score = 300000 + mvvLvaScore(board, move); // deprioritize bad captures
            else
                score = 900000 + mvvLvaScore(board, move);
        }
        // 3. Killer moves
        else if (std::find(killerMoves.begin(), killerMoves.end(), move) != killerMoves.end())
        {
            score = 800000;
        }
        // 4. History heuristic (optional)
        else if (historyHeuristic)
        {
            int from = move.from().index();
            int to = move.to().index();
            score = 1000 + historyHeuristic[from][to];
        }
        else if (board.givesCheck(move) != CheckType::NO_CHECK)
        {
            score += 500;
        }
        // 5. Quiet moves
        else
        {
            score = 0;
        }

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

void orderMovesInPlace(
    Board &board, Movelist &moves, int plyFromRoot,
    const std::optional<Move> &hashMove,
    const std::vector<Move> &killerMoves,
    long long historyHeuristic[64][64])
{
    auto moveScore = [&](const Move &move) -> int
    {
        int score = 0;
        if (hashMove && move == *hashMove)
            score = 1000000;
        else if (board.isCapture(move))
            score = 900000 + mvvLvaScore(board, move);
        else if ((!killerMoves.empty() && move == killerMoves[0]) ||
                 (killerMoves.size() > 1 && move == killerMoves[1]))
            score = 800000;
        else if (historyHeuristic)
            score = 1000 + historyHeuristic[move.from().index()][move.to().index()];
        else if (board.givesCheck(move) != CheckType::NO_CHECK)
            score += 500;
        else
            score = 0;
        return score;
    };

    std::sort(moves.begin(), moves.end(),
              [&](const Move &a, const Move &b)
              {
                  return moveScore(a) > moveScore(b);
              });
}