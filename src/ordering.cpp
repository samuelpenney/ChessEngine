#include "ordering.h"
#include <algorithm>

namespace {

// Ranks for MVV-LVA rather than real piece values: only the order matters.
// Indexed by PieceType (pawn, knight, bishop, rook, queen, king)
constexpr int RANK[6] = {1, 2, 3, 4, 5, 6};

int rank(PieceType t) {
    return RANK[static_cast<int>(t)];
}

bool sameMove(Move a, Move b) {
    return a.from() == b.from() && a.to() == b.to() && a.promo() == b.promo();
}

bool isTactical(Move m) {
    return m.captured() != Piece::None || m.promo() != PieceType::None;
}

// Returns moves sorted best first by moveScore, with first (if it's in the list) at the front
MoveList sortByScore(const MoveList &moves, Move first, bool tacticalOnly) {
    struct Scored {
        Move move;
        int score;
    };
    Scored scored[256];
    int n = 0;
    MoveList ordered;
    for (const Move &m : moves) {
        if (tacticalOnly && !isTactical(m)) continue;
        if (sameMove(m, first)) ordered.push(m);
        else scored[n++] = {m, moveScore(m)};
    }
    // Stable so moves with equal scores keep the generator's order, which keeps the search deterministic
    std::stable_sort(scored, scored + n, [](const Scored &a, const Scored &b) { return a.score > b.score; });
    for (int i = 0; i < n; ++i) ordered.push(scored[i].move);
    return ordered;
}

}

int moveScore(Move m) {
    int score = 0;
    // Most valuable victim first, then least valuable attacker: QxP comes after PxQ but before any quiet move
    if (m.captured() != Piece::None) score += 10 * rank(pieceType(m.captured())) - rank(pieceType(m.piece()));
    // A promotion is worth about as much as capturing the piece it becomes
    if (m.promo() != PieceType::None) score += 10 * rank(m.promo());
    return score;
}

MoveList orderMoves(const MoveList &moves, Move first) {
    return sortByScore(moves, first, false);
}

MoveList captures(const MoveList &moves) {
    return sortByScore(moves, Move(), true);
}
