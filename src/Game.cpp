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

                gameOver = __checkGameOver__(chessBoard);
                //gameOver = false; // For now, so this doesn't run into an infinite loop
                whiteTurn = !whiteTurn; // Switch turn
            }

        }

    private:

        bool __checkGameOver__(Board &chessBoard) {
            // TODO: implement
            const char *mover = whiteTurn ? "White" : "Black";
            const char *other = whiteTurn ? "Black" : "White";

            if (chessBoard.isCheckmate(whiteTurn)) {
                chessBoard.printBoard();
                std::cout << "\nCheckmate! " << other << " wins." << std::endl;
                return true;
            }
            if (chessBoard.isStalemate(whiteTurn)) {
                chessBoard.printBoard();
                std::cout << "\nStalemate! " << mover << " has no legal moves. The game is a draw." << std::endl;
                return true;
            }
            if (chessBoard.insufficientMaterial()) {
                chessBoard.printBoard();
                std::cout << "\nDraw by insufficient material." << std::endl;
                return true;
            }
            if (chessBoard.inCheck(whiteTurn)) {
                std::cout << "\nCheck! " << mover << " is in check." << std::endl;
            }
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


        // Now takes in promo from input
        bool moveFromInput(const std::string &input, Board &chessBoard) {
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
 
            const std::string &from = parts[0];
            const std::string &to = parts[1];
            int frSq = coordToSquare(from);
            int toSq = coordToSquare(to);
 
            if (frSq == -1 || toSq == -1) {
                std::cerr << "Invalid coordinates: " << from << ", " << to << std::endl;
                return false;
            }
 
            char promo = 0;
            if (parts.size() == 3) {
                if (parts[2].size() != 1 || std::string("QRBNqrbn").find(parts[2][0]) == std::string::npos) {
                    std::cerr << "Invalid promotion piece (use q, r, b or n): " << parts[2] << std::endl;
                    return false;
                }
                promo = static_cast<char>(std::toupper(parts[2][0]));
            }
 
            char piece = chessBoard.pieceCharAt(frSq);
            if (piece == '.') {
                std::cerr << "Illegal move: there is no piece on " << from << "." << std::endl;
                return false;
            }
            bool isWhitePiece = Board::isWhitePiece(piece);
            if (whiteTurn && !isWhitePiece) {
                std::cerr << "Illegal move: it's White's turn, but " << from << " has a black piece(" << piece << ")." << std::endl;
                return false;
            }
            if (!whiteTurn && isWhitePiece) {
                std::cerr << "Illegal move: it's Black's turn, but " << from << " has a white piece(" << piece << ")." << std::endl;
                return false;
            }
 
            // The move must be one of the legal moves for the side to move
            for (const Move &m : chessBoard.legalMoves(whiteTurn)) {
                if (m.from != frSq || m.to != toSq) continue;
                if (m.promo) {
                    char want = promo ? promo : 'Q';
                    if (m.promo != want) continue;
                }
                chessBoard.makeMove(m);
                return true;
            }
 
            if (chessBoard.inCheck(whiteTurn)) {
                std::cerr << "Illegal move: your king is in check and this move doesn't get it out of check." << std::endl;
            } else {
                std::cerr << "Illegal move: " << piece << " can't go from " << from << " to " << to
                          << " (blocked, wrong movement, or it would leave your king in check)." << std::endl;
            }
            return false;
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