#pragma once
#include "chess.hpp"

static const int MATERIAL_VALUES[6] = {100, 320, 330, 500, 900, 6000};
static const int MATE_SCORE = 69000;

static const chess::PieceType ptArray[6] = {
    chess::PieceType::PAWN,
    chess::PieceType::KNIGHT,
    chess::PieceType::BISHOP,
    chess::PieceType::ROOK,
    chess::PieceType::QUEEN,
    chess::PieceType::KING};

int evaluateBoard(const chess::Board &board, int plyFromRoot, chess::Movelist &moves);
int pawnStructure(const chess::Board &board, chess::Color color);
int kingSafety(const chess::Board &board, chess::Color color);
int mobility(const chess::Board &board, chess::Color color);
int countDoubledPawns(const chess::Board &board, chess::Color color);
int countIsolatedPawns(const chess::Board &board, chess::Color color);
int countPassedPawns(const chess::Board &board, chess::Color color);