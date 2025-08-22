#include "tt.hpp"
#include "eval.hpp"
using namespace chess;

// tt.cpp
#include "tt.hpp"
std::unordered_map<uint64_t, TTEntry> TT;

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