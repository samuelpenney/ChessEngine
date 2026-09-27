#include <iostream>
#include <cstdint>
#include <array>
#define BOARD_CPP

using Bitboard = uint64_t;

class Board {
    Bitboard whitePawns = 0;
    Bitboard whiteKnights = 0;
    Bitboard whiteRooks = 0;
    Bitboard whiteBishops = 0;
    Bitboard whiteKing = 0;
    Bitboard whiteQueens = 0;
    Bitboard blackPawns = 0;
    Bitboard blackKnights = 0;
    Bitboard blackRooks = 0;
    Bitboard blackBishops = 0;
    Bitboard blackKing = 0;
    Bitboard blackQueens = 0;

    public:
        static bool getBit(Bitboard bb, int square) {
            return (bb >> square) &1ULL;
        }

        void printBoard() const {
            for (int rank = 7; rank >= 0; --rank) {
                std::cout << (rank + 1) << " ";
                for (int file = 0; file < 8; ++file) {
                    int square = rank * 8 + file;
                    char piece = pieceCharAt(square);
                    std::cout << piece << ' ';
                }
                std::cout << '\n';
            }
            std::cout << "\n a b c d e f g h\n";
        }
    
    private:
        char pieceCharAt(int square) const {
            if (getBit(whitePawns, square)) return 'P';
            if (getBit(whiteKnights, square)) return 'N';
            
        }

};