#include <iostream>
#include "chess.hpp" // Disservin's chess library

int main() {
    chess::Board board; // Start position
    std::cout << "Starting FEN: " << board.getFen() << "\n";

    std::cout << "Legal moves: ";
    chess::Movelist moves;
    chess::movegen::legalmoves(moves, board);

    for (const chess::Move &move :  moves) {
        std::cout << move.move() << " ";
    }
    std::cout << "\n";
    return 0;
}