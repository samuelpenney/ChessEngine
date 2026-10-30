# Chess Engine

### Current Estimated ELO: 1521


## Engine


### Phase 3: Better evaluation

1. Piece-square tables, with separate middlegame and endgame tables blended by game phase (see PeSTO)

2. Pawn structure (doubled, isolated, passed), mobility, king safety, one at a time

### Phase 4: Speed and depth

1. Transposition table keyed by the Zobrist hash

2. Killer moves and history heuristic for ordering quiet moves

3. Principal variation search, null-move pruning, late move reductions

### Phase 5: Measure everything

1. UCI protocol so the engine runs in Cute Chess, Arena or lichess-bot

2. Self-play testing with cutechess-cli (SPRT once changes get small)

3. Track nodes per second and depth reached on fixed positions

Reference: https://www.chessprogramming.org
