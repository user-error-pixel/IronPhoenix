# Iron Phoenix

**Iron Phoenix** is a C++ chess engine built for **4-Player Chess Teams**.
The engine is designed around a fast mailbox board representation, team-based evaluation, alpha-beta search, transposition tables, and modern move-ordering techniques.

## Overview

Iron Phoenix is not a standard 2-player chess engine. It is built specifically for **4-player chess teams**, where:

- Red and Yellow are teammates.
- Blue and Green are teammates.
- Turn order is Red → Blue → Yellow → Green.
- The search evaluates positions from the current side’s team perspective.

The goal of the project is to build a strong, fast, and scalable 4PC Teams engine capable of deep search, reliable move ordering, and strong team-aware evaluation.

## Featrues

### Board Representation

- 14x16 mailbox board
- 224-square internal board
- Invalid mailbox squares for off-board detection
- Fast piece lists by color
- Piece index table for fast add, remove, and move operations
- King square tracking for each color

### Supported Colors and Teams

Red    = 1
Blue   = 2
Yellow = 3
Green  = 4

Team RY = Red + Yellow
Team BG = Blue + Green

### Move Generation

Iron Phoenix currently supports:

- Pawns
- Knights
- Bishops
- Rooks
- Queens
- Kings
- Promotions
- Captures
- Castling
- En passant
- Team-aware legality
- No same-team captures
- Legal move filtering using king safety

### Search

Iron Phoenix uses a negamax alpha-beta search designed for 4PC Teams.

Current search features include:

- Iterative deepening
- Alpha-beta pruning
- Quiescence search
- Transposition table probing
- Transposition table storing
- Principal variation tracking
- Time-controlled search using go movetime
- Depth-controlled search using go depth

### Move Ordering

Move ordering currently includes:

- Transposition table move
- Captures
- Promotions
- Killer moves
- Quiet move history
- Capture history

The intended ordering priority is:

1. TT move
2. Strong captures
3. Promotions
4. Killer moves
7. Quiet history moves
8. Remaining quiet moves
 
### Evaluation

The evaluation is team-relative.

Iron Phoenix evaluates positions from the perspective of the side to move’s team.

Current evaluation terms include:

Material
Mobility
Team mobility difference

Example idea:

```sh
score > 0  means good for the current side’s team
score < 0  means good for the enemy team
```

This allows normal negamax sign flipping to work because turn order alternates between teams.

### Zobrist Hashing

Iron Phoenix uses Zobrist hashing for fast position keys.

Hash components:

- Piece color
- Piece type
- Square
- Side to move

Important:

`initZobrist();`

must be called before pieces are added to the board.

Recommended initialization order:

```cpp
initZobrist();

Position pos;
initStartPosition(pos);

initPawnInfo();
initKnightInfo(pos);
initKingInfo(pos);

pos.key = computeZobristKey(pos);
```

### UCI-Like Commands

Iron Phoenix supports a simple UCI-style command loop.

Engine Setup

```sh
uci
isready
ucinewgame
Position Setup
position startpos
position startpos moves h2h3 b11b10
```

Search:

Search to a fixed depth:

`go depth 5`

Search for a fixed amount of time:

`go movetime 10000`

This searches for approximately 10 seconds.

Board Display

`d`

Perft

```sh
perft 4
divide 4
perftdebug 4
```

Quit

`quit`
