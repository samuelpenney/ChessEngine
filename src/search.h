#ifndef SEARCH_H
#define SEARCH_H
#include "Board.h"

constexpr int MATE = 100000;
constexpr int INF = 1000000;
// Iterative deepening never goes past this, e.g. when every line is a quick draw
constexpr int MAX_DEPTH = 64;
// Time the terminal and GUI engine players get per move
constexpr double ENGINE_SECONDS = 1.0;

struct SearchResult {
    Move move;
    int score;   // side to move's point of view
    int depth;   // deepest search that finished
    long nodes;
    double secs;
};

// Score from the side to move's point of view
int negamax(Board &board, int depth, int ply, int alpha, int beta);
// Only call these when the side to move has a legal move
Move findBestMove(Board &board, int depth);
// Iterative deepening: searches depth 1, 2, 3, ... until the time is up, and returns the
// result of the deepest search that finished
SearchResult searchForTime(Board &board, double seconds);
// How long to give searchForTime when playing on a clock with this much time left and this increment
double timeForMove(double remaining, double increment);

#endif
