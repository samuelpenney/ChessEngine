#include "test.h"
#include "search.h"
#include "evaluate.h"

namespace {

// Same rules as negamax but without pruning, so it looks at every move
int plainNegamax(Board &board, int depth, int ply) {
    if (ply > 0 && (board.repetitions() >= 1 || board.isFiftyMoveDraw() || board.insufficientMaterial())) return 0;
    MoveList moves = board.legalMoves();
    if (moves.empty()) return board.inCheck() ? -MATE + ply : 0;
    if (depth == 0) return evaluate(board);

    int best = -INF;
    for (const Move &m : moves) {
        Undo u = board.makeMove(m);
        int score = -plainNegamax(board, depth - 1, ply + 1);
        board.unmakeMove(m, u);
        if (score > best) best = score;
    }
    return best;
}

bool whiteMateInOne() {
    Board board = boardFromFen("6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1");
    CHECK_MOVE(findBestMove(board, 2), "a1a8");
    CHECK_EQ(negamax(board, 2, 0, -INF, INF), MATE - 1);
    return true;
}

bool blackMateInOne() {
    Board board = boardFromFen("r5k1/8/8/8/8/8/5PPP/6K1 b - - 0 1");
    CHECK_MOVE(findBestMove(board, 2), "a8a1");
    return true;
}

// Deeper searches also see slower mates; the + ply in the mate score makes the engine pick the fastest
bool prefersFasterMate() {
    Board board = boardFromFen("6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1");
    CHECK_MOVE(findBestMove(board, 4), "a1a8");
    CHECK_EQ(negamax(board, 4, 0, -INF, INF), MATE - 1);
    return true;
}

bool checkmatedScoresMinusMate() {
    Board board = boardFromFen("R5k1/5ppp/8/8/8/8/8/6K1 b - - 0 1");
    CHECK_EQ(negamax(board, 3, 0, -INF, INF), -MATE);
    return true;
}

// Black is down a queen but stalemated, so it's a draw rather than -900
bool stalemateScoresZero() {
    Board board = boardFromFen("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
    CHECK_EQ(negamax(board, 3, 0, -INF, INF), 0);
    return true;
}

bool repetitionScoresZero() {
    Board board;
    board.initPieces();
    for (const char *m : {"g1f3", "g8f6", "f3g1", "f6g8"}) CHECK(playMove(board, m));
    // Back at the start position; inside the search (ply > 0) that's a draw
    CHECK_EQ(negamax(board, 2, 1, -INF, INF), 0);
    return true;
}

bool capturesHangingQueen() {
    Board board = boardFromFen("4k3/8/8/3q4/8/8/8/3QK3 w - - 0 1");
    CHECK_MOVE(findBestMove(board, 3), "d1d5");
    return true;
}

bool avoidsDefendedPawn() {
    Board board = boardFromFen("4k3/8/2p5/3p4/8/8/8/3QK3 w - - 0 1");
    CHECK(moveName(findBestMove(board, 2)) != "d1d5");
    return true;
}

bool promotesToQueen() {
    Board board = boardFromFen("8/P6k/8/8/8/8/8/K7 w - - 0 1");
    CHECK_MOVE(findBestMove(board, 2), "a7a8q");
    return true;
}

// Alpha-beta only skips moves that can't change the result, so the score must match
bool alphaBetaMatchesPlainSearch() {
    const char *fens[] = {
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
    };
    for (const char *fen : fens) {
        Board board = boardFromFen(fen);
        CHECK_EQ(negamax(board, 3, 0, -INF, INF), plainNegamax(board, 3, 0));
    }
    return true;
}

bool searchLeavesBoardUnchanged() {
    Board board = boardFromFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    const uint64_t before = board.hash();
    findBestMove(board, 3);
    CHECK_EQ(board.hash(), before);
    CHECK_EQ(board.hash(), board.computeHash());
    return true;
}

}

std::vector<TestCase> searchTests() {
    return {
        {"search: white mate in 1", whiteMateInOne},
        {"search: black mate in 1", blackMateInOne},
        {"search: prefers the faster mate", prefersFasterMate},
        {"search: checkmated scores -MATE", checkmatedScoresMinusMate},
        {"search: stalemate scores 0", stalemateScoresZero},
        {"search: repetition scores 0", repetitionScoresZero},
        {"search: captures a hanging queen", capturesHangingQueen},
        {"search: avoids a defended pawn", avoidsDefendedPawn},
        {"search: promotes to a queen", promotesToQueen},
        {"search: alpha-beta matches plain search", alphaBetaMatchesPlainSearch},
        {"search: leaves the board unchanged", searchLeavesBoardUnchanged},
    };
}
