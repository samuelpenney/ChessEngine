#include "Game.h"
#include "search.h"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

static bool parseInput(const std::string &input, int &fromSq, int &toSq, PieceType &promo) {
    std::string s;
    s.reserve(input.size());
    for (char c : input) {
        if (c != ' ' && c != '\t') s.push_back(c);
    }

    std::vector<std::string> parts;
    size_t start = 0;
    while (true) {
        size_t comma = s.find(',', start);
        if (comma == std::string::npos) { parts.push_back(s.substr(start)); break; }
        parts.push_back(s.substr(start, comma - start));
        start = comma + 1;
    }
    if (parts.size() < 2 || parts.size() > 3) {
        std::cerr << "Invalid input (expected: from,to or from,to,promotion): " << input << std::endl;
        return false;
    }

    fromSq = Game::coordToSquare(parts[0]);
    toSq = Game::coordToSquare(parts[1]);

    if (fromSq == -1 || toSq == -1) {
        std::cerr << "Invalid coordinates: " << parts[0] << ", " << parts[1] << std::endl;
        return false;
    }

    promo = PieceType::None;
    if (parts.size() == 3) {
        if (parts[2].size() == 1) {
            switch (parts[2][0]) {
                case 'Q': case 'q': promo = PieceType::Queen; break;
                case 'R': case 'r': promo = PieceType::Rook; break;
                case 'B': case 'b': promo = PieceType::Bishop; break;
                case 'N': case 'n': promo = PieceType::Knight; break;
            }
        }
        if (promo == PieceType::None) {
            std::cerr << "Invalid promotion piece (use q, r, b or n): " << parts[2] << std::endl;
            return false;
        }
    }

    return true;
}

// Same format the player types, e.g. e7,e5 or a2,a1,q
static std::string formatMove(Move m) {
    auto square = [](int sq) {
        return std::string{static_cast<char>('a' + (sq & 7)), static_cast<char>('1' + (sq >> 3))};
    };
    std::string s = square(m.from()) + "," + square(m.to());
    if (m.promo() != PieceType::None) s += std::string(",") + "pnbrq"[static_cast<int>(m.promo())];
    return s;
}

// Returns false if input ended before the player picked a side
static bool askPlayerColor(bool &playerWhite) {
    while (true) {
        std::cout << "Play as White or Black? (w/b): " << std::endl;
        std::string input;
        if (!std::getline(std::cin, input) || input == "quit") return false;
        if (input == "w" || input == "W") { playerWhite = true; return true; }
        if (input == "b" || input == "B") { playerWhite = false; return true; }
        std::cerr << "Please type w or b." << std::endl;
    }
}

static void playEngineMove(Game &game) {
    std::cout << "Engine is thinking..." << std::endl;
    auto start = std::chrono::steady_clock::now();
    Move m = game.engineMove(ENGINE_DEPTH);
    double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

    std::string errorMsg;
    game.tryMove(m.from(), m.to(), m.promo(), errorMsg);
    std::cout << "Engine plays " << formatMove(m) << " (depth " << ENGINE_DEPTH << ", "
              << std::fixed << std::setprecision(3) << secs << "s)" << std::endl;
}

int main() {
    Game game;
    game.init();

    bool playerWhite;
    if (!askPlayerColor(playerWhite)) {
        std::cout << "Game ended." << std::endl;
        return 0;
    }
    game.getBoard().printBoard();

    while (true) {
        const char *name = game.isWhiteTurn() ? "White" : "Black";

        bool moveMade = game.isWhiteTurn() != playerWhite;
        if (moveMade) playEngineMove(game);
        while (!moveMade) {
            std::cout << name << " Move: " << std::endl;
            std::string input;
            if (!std::getline(std::cin, input) || input == "quit") {
                std::cout << "Game ended." << std::endl;
                return 0;
            }

            int fromSq, toSq;
            PieceType promo;
            if (!parseInput(input, fromSq, toSq, promo)) {
                std::cerr << "Try again: " << input << std::endl;
                continue;
            }

            std::string errorMsg;
            if (game.tryMove(fromSq, toSq, promo, errorMsg)) {
                moveMade = true;
            } else {
                std::cerr << errorMsg << std::endl;
                std::cerr << "Try again: " << input << std::endl;
            }
        }

        game.getBoard().printBoard();

        GameOverStatus status = game.checkGameOver();
        const char *mover = game.isWhiteTurn() ? "White" : "Black";
        const char *other = game.isWhiteTurn() ? "Black" : "White";

        switch (status) {
            case GameOverStatus::Checkmate:
                std::cout << "\nCheckmate! " << other << " wins." << std::endl;
                return 0;
            case GameOverStatus::Stalemate:
                std::cout << "\nStalemate! " << mover << " has no legal moves. The game is a draw." << std::endl;
                return 0;
            case GameOverStatus::InsufficientMaterial:
                std::cout << "\nDraw by insufficient material." << std::endl;
                return 0;
            case GameOverStatus::FiftyMoveRule:
                std::cout << "\nDraw by the fifty-move rule." << std::endl;
                return 0;
            case GameOverStatus::ThreefoldRepetition:
                std::cout << "\nDraw by threefold repetition." << std::endl;
                return 0;
            case GameOverStatus::InCheck:
                std::cout << "\nCheck! " << mover << " is in check." << std::endl;
                break;
            case GameOverStatus::Playing:
                break;
        }
    }
}
