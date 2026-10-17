// Perft: counts the positions reachable at a given depth and compares them with https://www.chessprogramming.org/Perft_Results
#include "Board.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace {

struct PerftTest {
    const char *name;
    const char *fen;
    int depth;
    long expected;
    long expectedDeep;  // at depth + 1, for --deep
};

const PerftTest TESTS[] = {
    {"start", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5, 4865609, 119060324},
    {"kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4, 4085603, 193690690},
    {"pos3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 5, 674624, 11030083},
    {"pos4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 4, 422333, 15833292},
    {"pos4-mirror", "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 4, 422333, 15833292},
    {"pos5", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 4, 2103487, 89941194},
    {"pos6", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 4, 3894594, 164075551},
};

long perft(Board &board, int depth) {
    if (depth == 0) return 1;
    const MoveList moves = board.legalMoves();
    if (depth == 1) return moves.size();

    long nodes = 0;
    for (Move m : moves) {
        Undo undo = board.makeMove(m);
        nodes += perft(board, depth - 1);
        board.unmakeMove(m, undo);
    }
    return nodes;
}

// Same walk without the depth 1 shortcut, checking the incremental hash at every node
// and that unmakeMove puts it back. Returns the number of mismatches.
long checkHashes(Board &board, int depth) {
    long bad = board.hash() != board.computeHash();
    if (depth == 0) return bad;

    const uint64_t before = board.hash();
    for (Move m : board.legalMoves()) {
        Undo undo = board.makeMove(m);
        bad += checkHashes(board, depth - 1);
        board.unmakeMove(m, undo);
        bad += board.hash() != before;
    }
    return bad;
}

int runHashCheck() {
    long bad = 0;
    for (const PerftTest &t : TESTS) {
        Board board;
        if (!board.loadFen(t.fen)) {
            std::printf("%-12s bad FEN\n", t.name);
            ++bad;
            continue;
        }
        const long n = checkHashes(board, t.depth - 1);
        std::printf("%-12s depth %d  %s\n", t.name, t.depth - 1, n ? "FAIL" : "ok");
        bad += n;
    }
    std::printf("\n%s\n", bad ? "Hash mismatches found" : "All hashes match");
    return bad ? 1 : 0;
}

std::string squareName(int sq) {
    return {static_cast<char>('a' + (sq & 7)), static_cast<char>('1' + (sq >> 3))};
}

// Long algebraic, e.g. e2e4 or a7a8q
std::string moveName(Move m) {
    std::string s = squareName(m.from()) + squareName(m.to());
    if (m.promo() != PieceType::None) s += "pnbrq"[static_cast<int>(m.promo())];
    return s;
}

double secondsSince(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

int runSuite(bool deep) {
    int failures = 0;
    long totalNodes = 0;
    double totalSecs = 0;
    for (const PerftTest &t : TESTS) {
        Board board;
        if (!board.loadFen(t.fen)) {
            std::printf("%-12s bad FEN\n", t.name);
            ++failures;
            continue;
        }
        const int depth = deep ? t.depth + 1 : t.depth;
        const long expected = deep ? t.expectedDeep : t.expected;

        auto start = std::chrono::steady_clock::now();
        long nodes = perft(board, depth);
        double secs = secondsSince(start);
        totalNodes += nodes;
        totalSecs += secs;

        bool ok = nodes == expected;
        if (!ok) ++failures;
        std::printf("%-12s depth %d %11ld  %-4s %6.2fs\n", t.name, depth, nodes, ok ? "ok" : "FAIL", secs);
        if (!ok) std::printf("             expected %11ld\n", expected);
    }
    std::printf("\n%s: %ld nodes in %.2fs (%.1f M nodes/s)\n", failures ? "FAILED" : "All passed",
                totalNodes, totalSecs, totalNodes / totalSecs / 1e6);
    return failures ? 1 : 0;
}

// Per-move counts at the root, for comparing against another engine to find a bug
int runPosition(const std::string &fen, int depth, bool divide) {
    Board board;
    if (!board.loadFen(fen)) {
        std::fprintf(stderr, "Invalid FEN: %s\n", fen.c_str());
        return 1;
    }

    auto start = std::chrono::steady_clock::now();
    long nodes = 0;
    if (divide && depth > 0) {
        for (Move m : board.legalMoves()) {
            Undo undo = board.makeMove(m);
            long n = perft(board, depth - 1);
            board.unmakeMove(m, undo);
            std::printf("%s: %ld\n", moveName(m).c_str(), n);
            nodes += n;
        }
        std::printf("\n");
    } else {
        nodes = perft(board, depth);
    }
    std::printf("Nodes: %ld (%.2fs)\n", nodes, secondsSince(start));
    return 0;
}

void usage(const char *prog) {
    std::fprintf(stderr,
                 "Usage:\n"
                 "  %s                          run the standard suite\n"
                 "  %s --deep                   run the suite one ply deeper (slower)\n"
                 "  %s --hash                   check the incremental position hash on the suite\n"
                 "  %s \"<fen>\" <depth> [--divide]  count one position, optionally per root move\n",
                 prog, prog, prog, prog);
}

}

int main(int argc, char **argv) {
    if (argc == 1) return runSuite(false);
    if (argc == 2 && std::string(argv[1]) == "--deep") return runSuite(true);
    if (argc == 2 && std::string(argv[1]) == "--hash") return runHashCheck();
    if (argc == 3 || (argc == 4 && std::string(argv[3]) == "--divide")) {
        char *end;
        long depth = std::strtol(argv[2], &end, 10);
        if (*end == '\0' && depth >= 0 && depth <= 12) return runPosition(argv[1], static_cast<int>(depth), argc == 4);
    }
    usage(argv[0]);
    return 2;
}
