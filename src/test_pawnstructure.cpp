#include <cassert>
#include <iostream>
#include "chess.hpp"
#include "eval.hpp"
using namespace chess;

bool testDoubledPawnsWhite()
{
    Board board;
    board.setFen("k7/5p2/5p2/8/7p/8/P1P5/K7 w - - 0 1");
    int doubledPawns = countDoubledPawns(board, Color::WHITE);
    int expected = 0;
    std::cout << "Doubled pawns (White): " << doubledPawns << ", Expected: " << expected << std::endl;
    return countDoubledPawns(board, Color::WHITE) == 0;
}

bool testDoubledPawnsBlack()
{
    Board board;
    board.setFen("k7/5p2/5p2/8/7p/8/P1P5/K7 w - - 0 1");
    return countDoubledPawns(board, Color::BLACK) == 1;
}

bool testIsolatedPawnsWhite()
{
    Board board;
    board.setFen("k7/5p2/5p2/8/7p/8/P1P5/K7 w - - 0 1");
    return countIsolatedPawns(board, Color::WHITE) == 2;
}

bool testIsolatedPawnsBlack()
{
    Board board;
    board.setFen("k7/5p2/5p2/8/7p/8/P1P5/K7 w - - 0 1");
    return countIsolatedPawns(board, Color::BLACK) == 3;
}

bool testPassedPawnsWhite()
{
    Board board;
    board.setFen("k7/5p2/5p2/8/7p/8/P1P5/K7 w - - 0 1");
    return countPassedPawns(board, Color::WHITE) == 2;
}

bool testPassedPawnsBlack()
{
    Board board;
    board.setFen("k7/5p2/5p2/8/7p/8/P1P5/K7 w - - 0 1");
    return countPassedPawns(board, Color::BLACK) == 2;
}

bool testPassedPawnsBlackBlockedbyKnight()
{
    Board board;
    board.setFen("k7/5p2/5p2/5n2/7p/8/P1P5/K7 w - - 0 1");
    return countPassedPawns(board, Color::BLACK) == 2;
}

int main(int argc, char *argv[])
{
    int passed = 0, total = 6;

    if (argc == 2)
    {
        std::string test = argv[1];
        if (test == "testDoubledPawnsWhite")
            return testDoubledPawnsWhite() ? 0 : 1;
        if (test == "testDoubledPawnsBlack")
            return testDoubledPawnsBlack() ? 0 : 1;
        if (test == "testIsolatedPawnsWhite")
            return testIsolatedPawnsWhite() ? 0 : 1;
        if (test == "testIsolatedPawnsBlack")
            return testIsolatedPawnsBlack() ? 0 : 1;
        if (test == "testPassedPawnsWhite")
            return testPassedPawnsWhite() ? 0 : 1;
        if (test == "testPassedPawnsBlack")
            return testPassedPawnsBlack() ? 0 : 1;
        if (test == "testPassedPawnsBlackBlockedbyKnight")
            return testPassedPawnsBlackBlockedbyKnight() ? 0 : 1;
        std::cout << "Unknown test: " << test << std::endl;
        return 2;
    }

    // Run all tests if no argument is given
    if (testDoubledPawnsWhite())
    {
        std::cout << "testDoubledPawnsWhite passed\n";
        ++passed;
    }
    else
        std::cout << "testDoubledPawnsWhite FAILED\n";

    if (testDoubledPawnsBlack())
    {
        std::cout << "testDoubledPawnsBlack passed\n";
        ++passed;
    }
    else
        std::cout << "testDoubledPawnsBlack FAILED\n";

    if (testIsolatedPawnsWhite())
    {
        std::cout << "testIsolatedPawnsWhite passed\n";
        ++passed;
    }
    else
        std::cout << "testIsolatedPawnsWhite FAILED\n";

    if (testIsolatedPawnsBlack())
    {
        std::cout << "testIsolatedPawnsBlack passed\n";
        ++passed;
    }
    else
        std::cout << "testIsolatedPawnsBlack FAILED\n";

    if (testPassedPawnsWhite())
    {
        std::cout << "testPassedPawnsWhite passed\n";
        ++passed;
    }
    else
        std::cout << "testPassedPawnsWhite FAILED\n";

    if (testPassedPawnsBlack())
    {
        std::cout << "testPassedPawnsBlack passed\n";
        ++passed;
    }
    else
        std::cout << "testPassedPawnsBlack FAILED\n";

    std::cout << passed << "/" << total << " tests passed.\n";
    return (passed == total) ? 0 : 1;
}