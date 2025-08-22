#pragma once
#include "chess.hpp"
#include <unordered_map>

struct TTEntry
{
    int depth;
    int value;
    chess::Move move;
    enum Flag
    {
        EXACT,
        LOWERBOUND,
        UPPERBOUND
    } flag;
};

extern std::unordered_map<uint64_t, TTEntry> TT;

std::optional<std::pair<int, chess::Move>> ttLookup(const chess::Board &board, int depth, int alpha, int beta, int plyFromRoot);
void ttStore(const chess::Board &board, int depth, chess::Move move, int value, int alpha, int beta, int plyFromRoot);
uint64_t boardKey(const chess::Board &board);