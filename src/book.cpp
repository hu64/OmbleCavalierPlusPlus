#include "book.hpp"
#include <fstream>
#include <optional>
#include <random>
using namespace chess;

static inline uint16_t be16(uint16_t x) { return (uint16_t)((x >> 8) | (x << 8)); }
static inline uint32_t be32(uint32_t x) { return ((x & 0xFF) << 24) | ((x & 0xFF00) << 8) | ((x & 0xFF0000) >> 8) | ((x >> 24) & 0xFF); }
static inline uint64_t be64(uint64_t x)
{
    return ((x & 0x00000000000000FFull) << 56) |
           ((x & 0x000000000000FF00ull) << 40) |
           ((x & 0x0000000000FF0000ull) << 24) |
           ((x & 0x00000000FF000000ull) << 8) |
           ((x & 0x000000FF00000000ull) >> 8) |
           ((x & 0x0000FF0000000000ull) >> 24) |
           ((x & 0x00FF000000000000ull) >> 40) |
           ((x & 0xFF00000000000000ull) >> 56);
}

bool loadPolyglotBook(const std::string &path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f)
    {
        std::cout << "info string Could not open book: " << path << "\n";
        return false;
    }
    BOOK.clear();
    while (true)
    {
        uint64_t k;
        uint16_t m;
        uint16_t w;
        uint32_t l;
        f.read(reinterpret_cast<char *>(&k), 8);
        if (!f)
            break;
        f.read(reinterpret_cast<char *>(&m), 2);
        f.read(reinterpret_cast<char *>(&w), 2);
        f.read(reinterpret_cast<char *>(&l), 4);
        PolyglotEntry raw;
        raw.key = be64(k);
        raw.move = be16(m);
        raw.weight = be16(w);
        raw.learn = be32(l);
        BOOK.emplace(raw.key, raw);
    }
    BOOK_LOADED = true;
    std::cout << "info string Loaded Polyglot book entries: " << BOOK.size() << " from " << path << "\n";
    return true;
}

// Polyglot move decoding helper
std::string polyglotMoveToUci(uint16_t move16)
{
    int from = (move16 >> 6) & 0x3F;
    int to = move16 & 0x3F;
    int promo = (move16 >> 12) & 0x7;

    std::string uci;
    uci += 'a' + (from % 8);
    uci += '1' + (from / 8);
    uci += 'a' + (to % 8);
    uci += '1' + (to / 8);

    if (promo)
    {
        // Polyglot: 1=knight, 2=bishop, 3=rook, 4=queen
        static const char promoChar[] = {' ', 'n', 'b', 'r', 'q'};
        uci += promoChar[promo];
    }
    return uci;
}

std::optional<Move> getBookMove(const Board &board)
{
    if (!BOOK_LOADED)
    {
        loadPolyglotBook(BOOK_PATH);
    }
    uint64_t key = board.hash();
    auto range = BOOK.equal_range(key);
    if (range.first == range.second)
        return std::nullopt;
    uint64_t total = 0;
    for (auto it = range.first; it != range.second; ++it)
        total += std::max<uint16_t>(1, it->second.weight);
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist(1, total);
    uint64_t pick = dist(gen);
    const PolyglotEntry *chosen = nullptr;
    for (auto it = range.first; it != range.second; ++it)
    {
        uint64_t w = std::max<uint16_t>(1, it->second.weight);
        if (pick <= w)
        {
            chosen = &it->second;
            break;
        }
        pick -= w;
    }
    if (!chosen)
        return std::nullopt;

    chess::Move chosenMove(chosen->move);
    std::string uciMove = polyglotMoveToUci(chosen->move);

    chess::Movelist legal;
    chess::movegen::legalmoves(legal, board);
    for (auto m : legal)
    {
        if (uci::moveToUci(m) == uciMove)
            return m;
    }
    return std::nullopt;
}