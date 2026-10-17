// Runs every test with its runtime, then times the search at increasing depths
#include "test.h"
#include "search.h"
#include <chrono>
#include <cstring>

namespace {

double secondsSince(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

int runTests() {
    std::vector<TestCase> tests = evaluateTests();
    for (const TestCase &t : searchTests()) tests.push_back(t);

    int failed = 0;
    double total = 0;
    for (const TestCase &t : tests) {
        auto start = std::chrono::steady_clock::now();
        bool ok = t.fn();
        double ms = secondsSince(start) * 1000;
        total += ms;
        if (!ok) ++failed;
        std::printf("%-4s %-45s %9.2f ms\n", ok ? "ok" : "FAIL", t.name, ms);
    }
    std::printf("\n%zu passed, %d failed in %.2f ms\n", tests.size() - failed, failed, total);
    return failed;
}

struct BenchPosition {
    const char *name;
    const char *fen;
    int maxDepth;
};

const BenchPosition BENCH[] = {
    {"start", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 6},
    {"kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 5},
    {"middlegame", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 5},
};

void runBenchmark() {
    std::printf("\nSearch times (findBestMove)\n");
    for (const BenchPosition &p : BENCH) {
        for (int depth = 1; depth <= p.maxDepth; ++depth) {
            Board board = boardFromFen(p.fen);
            auto start = std::chrono::steady_clock::now();
            Move best = findBestMove(board, depth);
            std::printf("%-12s depth %d  %-6s %9.3f s\n", p.name, depth, moveName(best).c_str(),
                        secondsSince(start));
        }
    }
}

}

int main(int argc, char **argv) {
    bool skipBench = argc == 2 && std::strcmp(argv[1], "--no-bench") == 0;
    int failed = runTests();
    if (!skipBench) runBenchmark();
    return failed ? 1 : 0;
}
