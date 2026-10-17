// Minimal test harness: each test is a function returning true on success
#ifndef TEST_H
#define TEST_H

#include "Board.h"
#include <cstdio>
#include <string>
#include <vector>

struct TestCase {
    const char *name;
    bool (*fn)();
};

#define CHECK(cond)                                                                   \
    do {                                                                              \
        if (!(cond)) {                                                                \
            std::printf("    %s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #cond);  \
            return false;                                                             \
        }                                                                             \
    } while (0)

#define CHECK_EQ(actual, expected)                                                         \
    do {                                                                                   \
        long long a_ = (actual), e_ = (expected);                                          \
        if (a_ != e_) {                                                                    \
            std::printf("    %s:%d: %s was %lld, expected %lld\n", __FILE__, __LINE__,     \
                        #actual, a_, e_);                                                  \
            return false;                                                                  \
        }                                                                                  \
    } while (0)

#define CHECK_MOVE(move, expected)                                                         \
    do {                                                                                   \
        std::string got_ = moveName(move);                                                 \
        if (got_ != (expected)) {                                                          \
            std::printf("    %s:%d: %s was %s, expected %s\n", __FILE__, __LINE__, #move,  \
                        got_.c_str(), expected);                                           \
            return false;                                                                  \
        }                                                                                  \
    } while (0)

inline std::string squareName(int sq) {
    return {static_cast<char>('a' + (sq & 7)), static_cast<char>('1' + (sq >> 3))};
}

// Long algebraic, e.g. e2e4 or a7a8q
inline std::string moveName(Move m) {
    std::string s = squareName(m.from()) + squareName(m.to());
    if (m.promo() != PieceType::None) s += "pnbrq"[static_cast<int>(m.promo())];
    return s;
}

inline Board boardFromFen(const char *fen) {
    Board board;
    if (!board.loadFen(fen)) std::printf("    bad FEN: %s\n", fen);
    return board;
}

// Plays a move given in long algebraic; false if it isn't legal
inline bool playMove(Board &board, const std::string &name) {
    for (Move m : board.legalMoves()) {
        if (moveName(m) == name) {
            board.makeMove(m);
            return true;
        }
    }
    return false;
}

std::vector<TestCase> evaluateTests();
std::vector<TestCase> searchTests();

#endif
