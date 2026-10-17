#include "evaluate.h"

constexpr int PIECE_VALUE[6] = {100, 320, 330, 500, 900, 0};

int evaluate(const Board &board) {
    int score = 0;
    for (int t = 0; t < 5; ++t) {
        PieceType type = static_cast<PieceType>(t);
        int white = __builtin_popcountll(board.pieces(makePiece(type, true)));
        int black = __builtin_popcountll(board.pieces(makePiece(type, false)));
        score += PIECE_VALUE[t] * (white - black);
    }
    return board.isWhiteTurn() ? score : -score;
}
