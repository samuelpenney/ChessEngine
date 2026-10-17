#include "Game.h"
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

int main() {
    Game game;
    game.init();
    game.getBoard().printBoard();

    while (true) {
        const char *name = game.isWhiteTurn() ? "White" : "Black";

        bool moveMade = false;
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
            case GameOverStatus::InCheck:
                std::cout << "\nCheck! " << mover << " is in check." << std::endl;
                break;
            case GameOverStatus::Playing:
                break;
        }
    }
}
