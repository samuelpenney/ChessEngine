# Chess Engine

## Things to do before engine

~~1. Implement game logic~~

~~2. Create a GUI~~

## Engine

### Phase 0: Finish the board

~~1. Move side to move into the board~~

~~2. Zobrist hashing (check with `make perft PERFT_ARGS=--hash`)~~

~~3. Halfmove clock and position history for the fifty-move rule and threefold repetition~~

### Phase 1: A minimal working engine

~~1. Material-only evaluation (P=100, N=320, B=330, R=500, Q=900), scored from the side to move's point of view~~

~~2. Negamax with alpha-beta at a fixed depth. Checkmate scores `-MATE + ply`, stalemate and repetition score 0~~

~~3. Let the engine play a side in the terminal game and the GUI, and display the evaluation in the GUI~~

### Phase 2: Make the search see clearly

1. Quiescence search: keep searching captures at depth 0 until the position is quiet

2. Move ordering: captures by MVV-LVA first, then quiet moves

3. Iterative deepening with a time limit

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
