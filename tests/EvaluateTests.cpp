#include "test.h"
#include "evaluate.h"

namespace {

bool startPositionIsEven() {
    Board board;
    board.initPieces();
    CHECK_EQ(evaluate(board), 0);
    return true;
}

bool kingsOnlyIsEven() {
    CHECK_EQ(evaluate(boardFromFen("4k3/8/8/8/8/8/8/4K3 w - - 0 1")), 0);
    return true;
}

bool pieceValues() {
    CHECK_EQ(evaluate(boardFromFen("4k3/8/8/8/8/8/P7/4K3 w - - 0 1")), 100);
    CHECK_EQ(evaluate(boardFromFen("4k3/8/8/8/8/8/8/3NK3 w - - 0 1")), 320);
    CHECK_EQ(evaluate(boardFromFen("4k3/8/8/8/8/8/8/3BK3 w - - 0 1")), 330);
    CHECK_EQ(evaluate(boardFromFen("4k3/8/8/8/8/8/8/3RK3 w - - 0 1")), 500);
    CHECK_EQ(evaluate(boardFromFen("4k3/8/8/8/8/8/8/3QK3 w - - 0 1")), 900);
    return true;
}

bool scoreIsFromSideToMove() {
    CHECK_EQ(evaluate(boardFromFen("4k3/8/8/8/8/8/8/3NK3 w - - 0 1")), 320);
    CHECK_EQ(evaluate(boardFromFen("4k3/8/8/8/8/8/8/3NK3 b - - 0 1")), -320);
    CHECK_EQ(evaluate(boardFromFen("3qk3/8/8/8/8/8/8/4K3 w - - 0 1")), -900);
    CHECK_EQ(evaluate(boardFromFen("3qk3/8/8/8/8/8/8/4K3 b - - 0 1")), 900);
    return true;
}

// Swapping colours and the side to move must give the same score
bool mirroredPositionsMatch() {
    Board a = boardFromFen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    Board b = boardFromFen("r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1");
    CHECK_EQ(evaluate(a), evaluate(b));
    return true;
}

}

std::vector<TestCase> evaluateTests() {
    return {
        {"evaluate: start position is 0", startPositionIsEven},
        {"evaluate: kings only is 0", kingsOnlyIsEven},
        {"evaluate: piece values", pieceValues},
        {"evaluate: side to move's point of view", scoreIsFromSideToMove},
        {"evaluate: mirrored positions match", mirroredPositionsMatch},
    };
}
