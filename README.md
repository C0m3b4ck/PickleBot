# PickleBot

**A chess bot made in C++, using algorithms and evaluation methods to play instead of a pre-trained ML model.**

Made for constrained environments, any forks (especially unique ones) very welcome.

**English** · [Polski](README.pl.md)

## How it works

### Board representation

A position is a `Board` struct holding an 8x8 piece array, the en passant target square, and the four castling rights. Squares are indexed `a8 = 0` ... `h1 = 63`.

### Move generation

Every candidate move is generated for each piece and validated against:

- standard movement rules (pawn, knight, bishop, rook, queen, king, en passant, castling),
- the square the mover is not allowed to leave its own king in check.

Only fully legal moves are ever searched or played.

### Evaluation

The static evaluation combines four components:

| Component      | Meaning                                          |
|----------------|--------------------------------------------------|
| Material       | piece values (`P=1, N/B=3, R=5, Q=9`) scaled     |
| Proximity      | how close the bot's pieces are to the enemy king |
| Check          | giving check to the enemy king                   |
| Expose         | moving a piece that leaves the bot's king exposed |

A `weighted()` function blends these into a single score according to the active **bot mode** (personality), so the same engine plays differently depending on the mode you pick.

### Search

- **Negamax** with **alpha-beta pruning**, running to depth 4 by default (up to 6).
- **Move ordering** by estimated value (captures first) for better pruning.
- **Iterative deepening**: depth increases turn by turn until the time budget runs out; the last completed depth is kept, so it always moves on time.
- **Time management**: the turn budget is a fraction of the remaining per-side clock, with a floor so it never stalls.
- **Opponent reply prediction** (verbose mode): after choosing a move, it searches a few plies deeper for the opponent's likely answer.

### Bot modes

1. **Aggressive** – hunts material.
2. **Offensive** – focuses on the enemy king.
3. **Defensive** – guards its own position.
4. **Guarding** – defensive focus on the king.

## Usage

Build with a C++17 compiler:

```sh
g++ -std=c++17 -O2 main.cpp board.cpp evaluate.cpp search.cpp -o picklebot
./picklebot
```

On startup you're asked for:

- the **bot mode** (1-4),
- your **side** (White / Black / Random),
- a **time limit** in seconds per side (0 for none),
- whether to enable **verbose mode** (shows the bot's thinking: move scores, search depth, time, predicted reply).

Moves are entered as algebraic squares, e.g. `e2 e4`.

## TODO

See [TODO.txt](TODO.txt): separate into files, finish logic, expand algorithm variables and weights.

## Credits

Started June 11th, 2026, by C0m3b4ck. Rewrite started on July 27th. As of now, there are no contributors other than the original author.
