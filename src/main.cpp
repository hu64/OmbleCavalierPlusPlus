#include "chess.hpp"

#include "puzzles.hpp"
#include "tt.hpp"
#include "book.hpp"
#include "search.hpp"
using namespace chess;

int main(int argc, char *argv[])
{

    // Check for test mode
    if (argc > 1 && std::string(argv[1]) == "--test")
    {
        if (argc < 5)
        {
            std::cerr << "Usage: " << argv[0] << " --test [FEN] [expected_move] [depth]" << std::endl;
            return 1;
        }
        std::string fen = argv[2];
        std::string expectedMove = argv[3];
        int depth = std::stoi(argv[4]);

        bool result = runSingleTest(fen, expectedMove, depth);
        return result ? 0 : 1; // Return success (0) only if test passes
    }

    Board board;
    std::string line;

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
            if (BOOK_LOADED || loadPolyglotBook(BOOK_PATH))
            {
                if (auto bm = getBookMove(board))
                {
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