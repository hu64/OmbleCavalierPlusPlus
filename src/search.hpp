#pragma once
#include "chess.hpp"
#include <chrono>

static const int MAX_DEPTH = 69;

struct SearchResult
{
    int score;
    chess::Move bestMove;
};

SearchResult negamaxRoot(chess::Board &board, int depth, int alpha, int beta,
                         std::chrono::steady_clock::time_point start, double timeLimit, int plyFromRoot, bool &timedOut);

int negamax(chess::Board &board, int depth, int alpha, int beta,
            std::chrono::steady_clock::time_point start, double timeLimit, int plyFromRoot, bool &timedOut);

int quiesce(chess::Board &board, int alpha, int beta, int plyFromRoot);

chess::Move findBestMoveIterative(chess::Board &board, int maxDepth, double totalTimeRemaining, double increment = 0.0);