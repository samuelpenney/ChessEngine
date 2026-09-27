#include <iostream>
#include <cstdint>
#include <array>
#include <string>
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

                gameOver = true; // For now, so this doesn't run into an infinite loop
                
                whiteTurn = !whiteTurn; // Switch turn
            }

        }

    private:
        void __gameInit__(Board &chessBoard) {
            chessBoard.initPieces();
            chessBoard.printBoard();
        }

};

int main() {
    Game game;
    game.gamePlay();
}