// Move helpers for the match runner
#ifndef TEST_H
#define TEST_H

#include "Board.h"
#include <string>

inline std::string squareName(int sq) {
    return {static_cast<char>('a' + (sq & 7)), static_cast<char>('1' + (sq >> 3))};
}

// Long algebraic, e.g. e2e4 or a7a8q, the format UCI uses
inline std::string moveName(Move m) {
    std::string s = squareName(m.from()) + squareName(m.to());
    if (m.promo() != PieceType::None) s += "pnbrq"[static_cast<int>(m.promo())];
    return s;
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

#endif
