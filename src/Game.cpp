#include <iostream>
#include <cstdint>
#include <array>
#include <string>
#include <vector>
#include <cctype>
#include <algorithm>
#include "Board.cpp"
#define GAME_CPP

class Game {
    bool whiteTurn = true;
    
    public:
        void gamePlay() {
            Board chessBoard;
            __gameInit__(chessBoard);

            bool gameOver = false;
            while (!gameOver) {
                chessBoard.printBoard();

                const char *name = whiteTurn ? "White" : "Black"; // Doing this to remove repetative code
                do {
                    std::cout << name << " Move: " << std::endl;
                    std::string input;
                    if (!std::getline(std::cin, input) || input =="quit") {
                        std::cout << "Game ended." << std::endl;
                        return;
                    }
                    if (moveFromInput(input, chessBoard)) break;
                    std::cerr << "Try again: " << input << std::endl;
                } while (true);

                // gameOver = __checkGameOver__(chessBoard);
                gameOver = false; // For now, so this doesn't run into an infinite loop
                whiteTurn = !whiteTurn; // Switch turn
            }

        }

    private:

        bool __checkGameOver__(Board &chessBoard) {
            // TODO: implement
            return false;
        }
        
        static int coordToSquare(const std::string &coord) {
            if (coord.size() != 2) {
                return -1; // Invalid coordinate
            }

            char fileChar = coord[0];
            char rankChar = coord[1];

            if (fileChar >= 'a' && fileChar <= 'h') fileChar = static_cast<char>(fileChar - 'a' + 'A');

            if (fileChar < 'A' || fileChar > 'H') return -1;
            if (rankChar < '1' || rankChar > '8') return -1;

            int file = fileChar - 'A'; // 0 .. 7
            int rank = rankChar - '1'; // 0 .. 7

            return rank * 8 + file;
        }

        bool moveFromInput(const std::string &input, Board &chessBoard) {
            std::string s;
            s.reserve(input.size());
            for (char c : input) {
                if (c != ' ' && c != '\t') s.push_back(c);
            }
            size_t commaPos = s.find(',');
            if (commaPos == std::string::npos) {
                std::cerr << "Invalide input (no comma): " << input << std::endl;
                return false;
            }

            std::string from = s.substr(0, commaPos);
            std::string to = s.substr(commaPos + 1);

            int frSq = coordToSquare(from);
            int toSq = coordToSquare(to);

            if (frSq == -1 || toSq == -1) {
                std::cerr << "Invalid coordinates: " << from << ", " << to << std::endl;
                return false;
            }
            char piece = chessBoard.pieceCharAt(frSq);
            bool isWhitePiece = (piece >= 'A' && piece <= 'Z');
            if (whiteTurn && !isWhitePiece) {
                std::cerr << "Illegal move: it's White's turn, but " << from << " has a black piece(" << piece << ")." << std::endl;
                return false;
            }
            if (!whiteTurn && isWhitePiece) {
                std::cerr << "Illegal move: it's Black's turn, but " << from << " has a white piece(" << piece << ")." << std::endl;
                return false;
            }

            chessBoard.movePiece(frSq, toSq);

            return true;
        }

        void __gameInit__(Board &chessBoard) {
            chessBoard.initPieces();
            chessBoard.printBoard();
        }

};

int main() {
    Game game;
    game.gamePlay();
}