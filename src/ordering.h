#ifndef ORDERING_H
#define ORDERING_H
#include "Board.h"


int moveScore(Move m);


MoveList orderMoves(const MoveList &moves, Move first = Move());
MoveList captures(const MoveList &moves);

#endif
