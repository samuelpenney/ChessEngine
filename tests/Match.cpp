// Plays the engine against Stockfish at fixed strengths (UCI_Elo) and estimates its rating from the results.
// Stockfish's UCI_Elo is calibrated at 60s + 0.6s per move, so that's the default time control.
// Usage: match [--tc <base>+<inc> | --time <seconds per move>] [--threads <games at once>] [elo ...]
//        match --watch [--black] [--tc ... | --time ...] [elo]   plays one game, showing every move
#include "test.h"
#include "search.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <spawn.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

extern char **environ;

namespace {

// Short openings so the games don't all repeat; each is played once with each colour
const char *OPENINGS[] = {
    "e2e4 e7e5", "d2d4 d7d5", "e2e4 c7c5", "e2e4 e7e6", "e2e4 c7c6",
    "d2d4 g8f6", "c2c4 e7e5", "g1f3 d7d5", "e2e4 e7e5 g1f3 b8c6", "d2d4 d7d5 c2c4 e7e6",
};
struct TimeControl {
    double base = 60, inc = 0.6;  // seconds on each clock, and added after each move
    double movetime = 0;          // when > 0, a fixed time per move and no clocks
};

std::string describe(const TimeControl &tc) {
    char buf[64];
    if (tc.movetime > 0) std::snprintf(buf, sizeof(buf), "%.2f s per move", tc.movetime);
    else std::snprintf(buf, sizeof(buf), "%g+%g", tc.base, tc.inc);
    return buf;
}

// Games this long are called a draw, e.g. when neither side can make progress
constexpr int MAX_PLIES = 400;

// A Stockfish process spoken to over UCI through a pair of pipes
class Stockfish {
    pid_t pid = -1;
    FILE *toEngine = nullptr;
    FILE *fromEngine = nullptr;

public:
    bool start(int elo) {
        int in[2], out[2];
        if (pipe(in) != 0 || pipe(out) != 0) return false;
        posix_spawn_file_actions_t actions;
        posix_spawn_file_actions_init(&actions);
        posix_spawn_file_actions_adddup2(&actions, in[0], STDIN_FILENO);
        posix_spawn_file_actions_adddup2(&actions, out[1], STDOUT_FILENO);
        for (int fd : {in[0], in[1], out[0], out[1]}) posix_spawn_file_actions_addclose(&actions, fd);
        char *argv[] = {const_cast<char *>("stockfish"), nullptr};
        int err = posix_spawnp(&pid, "stockfish", &actions, nullptr, argv, environ);
        posix_spawn_file_actions_destroy(&actions);
        close(in[0]);
        close(out[1]);
        if (err != 0) return false;
        toEngine = fdopen(in[1], "w");
        fromEngine = fdopen(out[0], "r");

        send("uci");
        waitFor("uciok");
        send("setoption name Threads value 1");
        send("setoption name Hash value 16");
        send("setoption name UCI_LimitStrength value true");
        send("setoption name UCI_Elo value " + std::to_string(elo));
        send("ucinewgame");
        send("isready");
        return !waitFor("readyok").empty();
    }

    ~Stockfish() {
        if (pid == -1) return;
        send("quit");
        fclose(toEngine);
        fclose(fromEngine);
        waitpid(pid, nullptr, 0);
    }

    void send(const std::string &cmd) {
        std::fprintf(toEngine, "%s\n", cmd.c_str());
        std::fflush(toEngine);
    }

    // Reads lines until one starts with prefix; empty if the process went away
    std::string waitFor(const std::string &prefix) {
        char buf[4096];
        while (std::fgets(buf, sizeof(buf), fromEngine)) {
            std::string line(buf);
            if (line.compare(0, prefix.size(), prefix) == 0) return line;
        }
        return "";
    }

    // goArgs is the rest of the UCI go command, e.g. "movetime 1000" or "wtime ... btime ..."
    std::string bestMove(const std::string &moves, const std::string &goArgs) {
        send("position startpos moves " + moves);
        send("go " + goArgs);
        std::string line = waitFor("bestmove ");
        if (line.empty()) return "";
        std::string move = line.substr(9);
        return move.substr(0, move.find_first_of(" \n"));
    }
};

struct Job {
    int elo;
    const char *opening;
    bool engineWhite;
};

struct GameResult {
    double score = 0;  // for our engine: 1 win, 0.5 draw, 0 loss
    std::string reason;
    int plies = 0;
    Board finalBoard;
    std::string moves;  // the whole game, opening included
};

// Redraws the board in place, with who played the last move and what the engine thought of it
void showPosition(const Board &board, const Job &job, const TimeControl &tc, const double clock[2], int plies,
                  const std::string &last, bool byEngine, const SearchResult &search) {
    std::printf("\033[2J\033[H");
    std::printf("Engine (%s) vs Stockfish %d   opening %s   %s\n", job.engineWhite ? "White" : "Black", job.elo,
                job.opening, describe(tc).c_str());
    if (tc.movetime == 0) std::printf("Clocks: White %.1fs  Black %.1fs\n", clock[0], clock[1]);
    if (!last.empty()) {
        std::printf("Ply %d: %s played %s", plies + 1, byEngine ? "engine" : "stockfish", last.c_str());
        if (byEngine) {
            std::printf("   (depth %d, score %+.2f, %.2fs)", search.depth, search.score / 100.0, search.secs);
        }
        std::printf("\n");
    }
    std::fflush(stdout);
    board.printBoard();
    std::cout << std::flush;
}

GameResult playGame(const Job &job, const TimeControl &tc, bool watch = false) {
    GameResult result;
    Stockfish sf;
    if (!sf.start(job.elo)) {
        result.reason = "could not start stockfish";
        return result;
    }

    Board board;
    board.initPieces();
    std::string moves = job.opening;
    auto finish = [&] {
        result.finalBoard = board;
        result.moves = moves;
        return result;
    };
    int openingPlies = 0;
    double clock[2] = {tc.base, tc.base};  // White, Black
    for (size_t pos = 0; pos < moves.size(); ++openingPlies) {
        size_t end = moves.find(' ', pos);
        if (end == std::string::npos) end = moves.size();
        playMove(board, moves.substr(pos, end - pos));
        pos = end + 1;
    }

    for (result.plies = 0;; ++result.plies) {
        bool engineToMove = board.isWhiteTurn() == job.engineWhite;
        if (board.legalMoves().empty()) {
            if (board.inCheck()) {
                result.score = engineToMove ? 0 : 1;
                result.reason = "checkmate";
            } else {
                result.score = 0.5;
                result.reason = "stalemate";
            }
            return finish();
        }
        result.score = 0.5;
        if (board.insufficientMaterial()) { result.reason = "insufficient material"; return finish(); }
        if (board.repetitions() >= 2) { result.reason = "repetition"; return finish(); }
        if (board.isFiftyMoveDraw()) { result.reason = "fifty moves"; return finish(); }
        if (result.plies >= MAX_PLIES) { result.reason = "move limit"; return finish(); }

        if (watch && result.plies == 0) showPosition(board, job, tc, clock, openingPlies, "", false, {});

        double &myClock = clock[board.isWhiteTurn() ? 0 : 1];
        auto start = std::chrono::steady_clock::now();
        std::string name;
        SearchResult search{};
        if (engineToMove) {
            search = searchForTime(board, tc.movetime > 0 ? tc.movetime : timeForMove(myClock, tc.inc));
            name = moveName(search.move);
        } else if (tc.movetime > 0) {
            name = sf.bestMove(moves, "movetime " + std::to_string(static_cast<int>(tc.movetime * 1000)));
        } else {
            auto ms = [](double secs) { return std::to_string(static_cast<long>(secs * 1000)); };
            name = sf.bestMove(moves, "wtime " + ms(clock[0]) + " btime " + ms(clock[1]) + " winc " + ms(tc.inc) +
                                          " binc " + ms(tc.inc));
        }
        if (tc.movetime == 0) {
            myClock -= std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
            if (myClock < 0) {
                result.score = engineToMove ? 0 : 1;
                result.reason = engineToMove ? "engine lost on time" : "stockfish lost on time";
                return finish();
            }
            myClock += tc.inc;
        }
        if (!playMove(board, name)) {
            // Shouldn't happen; counts against whoever sent it so a bug can't look like a win
            result.score = engineToMove ? 0 : 1;
            result.reason = "illegal move " + name + (engineToMove ? " by engine" : " by stockfish");
            return finish();
        }
        moves += " " + name;
        if (watch) showPosition(board, job, tc, clock, openingPlies + result.plies, name, engineToMove, search);
    }
}

double expectedScore(double rating, double opponent) {
    return 1.0 / (1.0 + std::pow(10.0, (opponent - rating) / 400.0));
}

// The rating whose expected score against these opponents equals the actual score
double performanceRating(const std::vector<int> &opponents, double score) {
    double lo = -1000, hi = 5000;
    for (int i = 0; i < 100; ++i) {
        double mid = (lo + hi) / 2, expected = 0;
        for (int opp : opponents) expected += expectedScore(mid, opp);
        (expected < score ? lo : hi) = mid;
    }
    return (lo + hi) / 2;
}

}

int main(int argc, char **argv) {
    std::signal(SIGPIPE, SIG_IGN);
    TimeControl tc;
    int threads = 4;
    bool watch = false, engineWhite = true;
    std::vector<int> levels;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--watch") watch = true;
        else if (arg == "--black") engineWhite = false;
        else if (arg == "--time" && i + 1 < argc) tc.movetime = std::atof(argv[++i]);
        else if (arg == "--threads" && i + 1 < argc) threads = std::atoi(argv[++i]);
        else if (arg == "--tc" && i + 1 < argc && std::sscanf(argv[i + 1], "%lf+%lf", &tc.base, &tc.inc) == 2) {
            tc.movetime = 0;
            ++i;
        } else if (std::atoi(argv[i]) >= 1320) {
            levels.push_back(std::atoi(argv[i]));
        } else {
            std::fprintf(stderr,
                         "Usage: %s [--tc <base>+<inc> | --time <seconds per move>] [--threads <n>] [elo ...]\n"
                         "       %s --watch [--black] [--tc ... | --time ...] [elo]\n"
                         "Elo levels go from 1320 to 3190; the default time control is 60+0.6\n",
                         argv[0], argv[0]);
            return 2;
        }
    }
    if (watch) {
        Job job{levels.empty() ? 1320 : levels[0], OPENINGS[0], engineWhite};
        GameResult r = playGame(job, tc, true);
        std::printf("\nEngine %s: %s after %d plies\n", r.score == 1 ? "wins" : r.score == 0 ? "loses" : "draws",
                    r.reason.c_str(), r.plies);
        return 0;
    }
    if (levels.empty()) levels = {1320, 1500};

    std::vector<Job> jobs;
    for (int elo : levels)
        for (const char *opening : OPENINGS)
            for (bool white : {true, false}) jobs.push_back({elo, opening, white});

    std::printf("%zu games at %s, %d at a time\n\n", jobs.size(), describe(tc).c_str(), threads);
    std::vector<GameResult> results(jobs.size());
    std::atomic<size_t> next{0};
    std::mutex printLock;
    int done = 0;
    auto worker = [&] {
        for (size_t i; (i = next++) < jobs.size();) {
            results[i] = playGame(jobs[i], tc);
            std::lock_guard<std::mutex> lock(printLock);
            const Job &j = jobs[i];
            const GameResult &r = results[i];
            std::printf("[%3d/%zu] vs %d as %s  %-22s %-4s %-22s %d plies\n", ++done, jobs.size(), j.elo,
                        j.engineWhite ? "White" : "Black", j.opening,
                        r.score == 1 ? "win" : r.score == 0 ? "loss" : "draw", r.reason.c_str(), r.plies);
            std::fflush(stdout);
            r.finalBoard.printBoard();
            std::cout << "Moves: " << r.moves << "\n\n" << std::flush;
        }
    };
    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t) pool.emplace_back(worker);
    for (std::thread &t : pool) t.join();

    std::printf("\n");
    std::vector<int> opponents;
    double total = 0, sumSquares = 0;
    for (int elo : levels) {
        int w = 0, d = 0, l = 0;
        for (size_t i = 0; i < jobs.size(); ++i) {
            if (jobs[i].elo != elo) continue;
            double s = results[i].score;
            (s == 1 ? w : s == 0 ? l : d)++;
            opponents.push_back(elo);
            total += s;
            sumSquares += s * s;
        }
        std::printf("vs Stockfish %d: +%d =%d -%d  (%.1f%%)\n", elo, w, d, l, 100.0 * (w + 0.5 * d) / (w + d + l));
    }

    const int n = static_cast<int>(opponents.size());
    if (total == 0 || total == n) {
        std::printf("\nScored %s, so the rating is only known to be %s %d. Try %s levels.\n",
                    total == 0 ? "0%" : "100%", total == 0 ? "below about" : "above about",
                    total == 0 ? *std::min_element(levels.begin(), levels.end())
                               : *std::max_element(levels.begin(), levels.end()),
                    total == 0 ? "lower (or --time longer)" : "higher");
        return 0;
    }
    // 95% range from the spread of the game results
    double mean = total / n;
    double stderrScore = std::sqrt(std::max(0.0, sumSquares / n - mean * mean) / n);
    double lo = performanceRating(opponents, std::max(0.0, mean - 1.96 * stderrScore) * n);
    double hi = performanceRating(opponents, std::min(1.0, mean + 1.96 * stderrScore) * n);
    std::printf("\nEstimated rating: %.0f  (95%% range %.0f to %.0f, %d games)\n",
                performanceRating(opponents, total), lo, hi, n);
    return 0;
}
