#pragma once
#include "chess.hpp"
#include <optional>
#include <iostream>
#include <sstream>
#include <fstream>

struct PolyglotEntry
{
    uint64_t key;
    uint16_t move;
    uint16_t weight;
    uint32_t learn;
};

static std::unordered_multimap<uint64_t, PolyglotEntry> BOOK;
static bool BOOK_LOADED = false;
static std::string BOOK_PATH = "baron30.bin";

bool loadPolyglotBook(const std::string &path);
std::optional<chess::Move> getBookMove(const chess::Board &board);