#ifndef SEARCH_H
#define SEARCH_H
#include "Board.h"

constexpr int MATE = 100000;
constexpr int INF = 1000000;
// Depth the terminal and GUI engine players search to
constexpr int ENGINE_DEPTH = 5;

// Score from the side to move's point of view
int negamax(Board &board, int depth, int ply, int alpha, int beta);
// Only call this when the side to move has a legal move
Move findBestMove(Board &board, int depth);

#endif
