#include "search.h"
#include "evaluate.h"
#include <algorithm>
#include <chrono>
#include <cstdlib>

namespace {

using Clock = std::chrono::steady_clock;

// State of the current timed search. thread_local so the GUI's search thread has its own copy
thread_local bool timed = false;
thread_local bool stopped = false;
thread_local Clock::time_point deadline;
thread_local long nodes = 0;

// Reading the clock at every node would slow the search down, so check every 2048 nodes
bool timeUp() {
    ++nodes;
    if (timed && (nodes & 2047) == 0 && Clock::now() >= deadline) stopped = true;
    return stopped;
}

double secondsSince(Clock::time_point start) {
    return std::chrono::duration<double>(Clock::now() - start).count();
}

bool sameMove(Move a, Move b) {
    return a.from() == b.from() && a.to() == b.to() && a.promo() == b.promo();
}

// Searches every root move to depth, trying first before the others. Returns false if the
// time ran out part way, in which case best and bestScore are left alone
bool searchRoot(Board &board, const MoveList &moves, int depth, Move first, Move &best, int &bestScore) {
    MoveList ordered;
    for (const Move &m : moves) if (sameMove(m, first)) ordered.push(m);
    for (const Move &m : moves) if (!sameMove(m, first)) ordered.push(m);

    Move bestSoFar = ordered[0];
    int alpha = -INF;
    for (const Move &m : ordered) {
        Undo u = board.makeMove(m);
        int score = -negamax(board, depth - 1, 1, -INF, -alpha);
        board.unmakeMove(m, u);
        if (stopped) return false;
        if (score > alpha) {
            alpha = score;
            bestSoFar = m;
        }
    }
    best = bestSoFar;
    bestScore = alpha;
    return true;
}

}

int negamax(Board &board, int depth, int ply, int alpha, int beta ) {
    // The score is thrown away once the time is up, so any value will do
    if (timeUp()) return 0;

    if (ply > 0 && (board.repetitions() >= 1 || board.isFiftyMoveDraw() || board.insufficientMaterial())) return 0;

    MoveList moves = board.legalMoves();
    if (moves.empty()) return board.inCheck() ? -MATE + ply : 0;

    if (depth == 0) return evaluate(board);

    for (const Move &m : moves) {
        Undo u = board.makeMove(m);
        int score = -negamax(board, depth - 1, ply + 1, -beta, -alpha);
        board.unmakeMove(m, u);
        if (stopped) return 0;

        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }
    return alpha;
}

Move findBestMove(Board &board, int depth) {
    timed = stopped = false;
    Move best;
    int score;
    searchRoot(board, board.legalMoves(), depth, Move(), best, score);
    return best;
}

double timeForMove(double remaining, double increment) {
    // searchForTime usually finishes after about half its limit, so this spends roughly
    // 1/40 of the clock plus the increment per move, which spreads it over the rest of the game
    double limit = remaining / 20 + increment;
    // Never risk more than a quarter of what's left, and keep a little back for move overhead
    return std::max(0.01, std::min(limit, remaining / 4 - 0.05));
}

SearchResult searchForTime(Board &board, double seconds) {
    const Clock::time_point start = Clock::now();
    deadline = start + std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(seconds));
    timed = true;
    stopped = false;
    nodes = 0;

    const MoveList moves = board.legalMoves();
    // A legal move to fall back on in case not even depth 1 finishes
    SearchResult result{moves[0], 0, 0, 0, 0};
    for (int depth = 1; depth <= MAX_DEPTH; ++depth) {
        Move best;
        int score;
        // Start with the previous depth's best move, so the window gets narrow early and more gets pruned
        if (!searchRoot(board, moves, depth, result.move, best, score)) break;
        result.move = best;
        result.score = score;
        result.depth = depth;

        // A mate already found can't get any better, a single legal move needs no thinking, and each
        // depth takes several times longer than the last, so past half the time the next one wouldn't finish
        if (std::abs(score) >= MATE - MAX_DEPTH || moves.size() == 1 || secondsSince(start) > seconds / 2) break;
    }

    result.nodes = nodes;
    result.secs = secondsSince(start);
    timed = stopped = false;
    return result;
}
