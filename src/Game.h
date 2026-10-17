#ifndef GAME_H
#define GAME_H

#include "Board.h"
#include <string>

enum class GameOverStatus {
    Playing,
    InCheck,
    Checkmate,
    Stalemate,
    InsufficientMaterial,
    FiftyMoveRule,
    ThreefoldRepetition
};

class Game {
    Board board;
    MoveList legal;  // legal moves for the side to move, refreshed after every move

public:
    void init();
    const Board &getBoard() const;
    bool isWhiteTurn() const;
    const MoveList &legalMoves() const;
    GameOverStatus checkGameOver() const;
    bool tryMove(int fromSq, int toSq, PieceType promo, std::string &errorMsg);
    static int coordToSquare(const std::string &coord);
};

#endif
