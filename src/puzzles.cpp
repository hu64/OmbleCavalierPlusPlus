#include "puzzles.hpp"
#include "search.hpp"
#include "tt.hpp"
using namespace chess;


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

bool runSingleTest(const std::string &fen, const std::string &expectedMove, int depth)
{
    Board board;
    board.setFen(fen);
    TT.clear();

    // Allocate plenty of time for the test
    Move bestMove = findBestMoveIterative(board, depth, 60.0);
    std::string bestMoveUci = uci::moveToUci(bestMove);

    bool passed = (bestMoveUci == expectedMove);

    if (passed)
    {
        std::cout << "[PASS] Found best move: " << bestMoveUci << std::endl;
    }
    else
    {
        std::cout << "[FAIL] Expected: " << expectedMove << ", Got: " << bestMoveUci << std::endl;
    }

    return passed;
}
