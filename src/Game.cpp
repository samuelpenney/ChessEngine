#include "Game.h"
#include <cctype>

void Game::init() {
    board.initPieces();
    whiteTurn = true;
}

const Board &Game::getBoard() const {
    return board;
}

bool Game::isWhiteTurn() const {
    return whiteTurn;
}

GameOverStatus Game::checkGameOver() const {
    if (board.isCheckmate(whiteTurn)) {
        return GameOverStatus::Checkmate;
    }
    if (board.isStalemate(whiteTurn)) {
        return GameOverStatus::Stalemate;
    }
    if (board.insufficientMaterial()) {
        return GameOverStatus::InsufficientMaterial;
    }
    if (board.inCheck(whiteTurn)) {
        return GameOverStatus::InCheck;
    }
    return GameOverStatus::Playing;
}

bool Game::tryMove(int fromSq, int toSq, char promo, std::string &errorMsg) {
    char piece = board.pieceCharAt(fromSq);
    if (piece == '.') {
        errorMsg = "Illegal move: there is no piece on that square.";
        return false;
    }
    bool isWhite = Board::isWhitePiece(piece);
    if (whiteTurn && !isWhite) {
        errorMsg = "Illegal move: it's White's turn, but that square has a black piece.";
        return false;
    }
    if (!whiteTurn && isWhite) {
        errorMsg = "Illegal move: it's Black's turn, but that square has a white piece.";
        return false;
    }

    for (const Move &m : board.legalMoves(whiteTurn)) {
        if (m.from != fromSq || m.to != toSq) continue;
        if (m.promo) {
            char want = promo ? static_cast<char>(std::toupper(promo)) : 'Q';
            if (m.promo != want) continue;
        }
        board.makeMove(m);
        whiteTurn = !whiteTurn;
        return true;
    }

    if (board.inCheck(whiteTurn)) {
        errorMsg = "Illegal move: your king is in check and this move doesn't get it out of check.";
    } else {
        errorMsg = "Illegal move: that piece can't move there (blocked, wrong movement, or it would leave your king in check).";
    }
    return false;
}

int Game::coordToSquare(const std::string &coord) {
    if (coord.size() != 2) {
        return -1;
    }

    char fileChar = coord[0];
    char rankChar = coord[1];

    if (fileChar >= 'a' && fileChar <= 'h') fileChar = static_cast<char>(fileChar - 'a' + 'A');

    if (fileChar < 'A' || fileChar > 'H') return -1;
    if (rankChar < '1' || rankChar > '8') return -1;

    int file = fileChar - 'A';
    int rank = rankChar - '1';

    return rank * 8 + file;
}
