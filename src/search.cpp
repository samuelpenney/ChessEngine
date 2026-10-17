#include "search.h"
#include "evaluate.h"

int negamax(Board &board, int depth, int ply, int alpha, int beta ) {
    
    if (ply > 0 && (board.repetitions() >= 1 || board.isFiftyMoveDraw() || board.insufficientMaterial())) return 0;

    MoveList moves = board.legalMoves();
    if (moves.empty()) return board.inCheck() ? -MATE + ply : 0;

    if (depth == 0) return evaluate(board);

    for (const Move &m : moves) {
        Undo u = board.makeMove(m);
        int score = -negamax(board, depth - 1, ply + 1, -beta, -alpha);
        board.unmakeMove(m, u);

        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }
    return alpha;
}

Move findBestMove(Board &board, int depth) {
    
    Move best;
    int alpha = -INF;
    for (const Move &m : board.legalMoves()) {
        Undo u = board.makeMove(m);
        int score = -negamax(board, depth - 1, 1, -INF, -alpha);
        board.unmakeMove(m, u);
        if (score > alpha) {
            alpha = score;
            best = m;
        }
    }
    return best;
}