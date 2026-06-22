#pragma once
#include <cstdint>
#include <cstring>
#include <algorithm>

using Score = int;
using Depth = int;

constexpr int BOARD_SIZE = 224;
constexpr int MAX_MOVES = 512;
constexpr int MAX_PIECES_PER_COLOR = 32;

constexpr int MAILBOX_WIDTH = 16;

constexpr int RED_PROMO_ROW = 3;
constexpr int YELLOW_PROMO_ROW = 10;
constexpr int BLUE_PROMO_COL = 11;
constexpr int GREEN_PROMO_COL = 4;

constexpr int knightOffsets[8] = {
    -18, -33,
    -31, -14,
     18,  33,
     31,  14
};

constexpr int DIR_N = -16;
constexpr int DIR_S = 16;
constexpr int DIR_E = 1;
constexpr int DIR_W = -1;

constexpr int DIR_NE = DIR_N + DIR_E;
constexpr int DIR_NW = DIR_N + DIR_W;
constexpr int DIR_SE = DIR_S + DIR_E;
constexpr int DIR_SW = DIR_S + DIR_W;

constexpr int rookDirs[4] = {
    DIR_N, DIR_S, DIR_E, DIR_W
};

constexpr int bishopDirs[4] = {
    DIR_NE, DIR_NW, DIR_SE, DIR_SW
};

constexpr int queenDirs[8] = {
    DIR_N, DIR_S, DIR_E, DIR_W,
    DIR_NE, DIR_NW, DIR_SE, DIR_SW
};

constexpr int kingOffsets[8] = {
    DIR_N, DIR_S, DIR_E, DIR_W,
    DIR_NE, DIR_NW, DIR_SE, DIR_SW
};

constexpr Score INF = 30000000;
constexpr Score MATE_SCORE = 29000000;

constexpr int BOARD_WIDTH = 16;
constexpr int BOARD_RANKS = 14;

constexpr int CASTLE_KINGSIDE = 0;
constexpr int CASTLE_QUEENSIDE = 1;

extern uint64_t zobristPiece[5][7][BOARD_SIZE];
extern uint64_t zobristEp[5][BOARD_SIZE];
extern uint64_t zobristCastle[5][2];
extern uint64_t zobristTurn[5];

void initZobrist();

extern int knightMoveCount[BOARD_SIZE];
extern int knightMoves[BOARD_SIZE][8];

extern int kingMoveCount[BOARD_SIZE];
extern int kingMoves[BOARD_SIZE][8];

enum Color : int {
    NO_COLOR = 0,
    RED = 1,
    BLUE = 2,
    YELLOW = 3,
    GREEN = 4
};

enum Team : int {
    NO_TEAM = 0,
    TEAM_RY = 1,
    TEAM_BG = 2
};

enum Piece : int {
    EMPTY = 0,
    PAWN = 1,
    KNIGHT = 2,
    BISHOP = 3,
    ROOK = 4,
    QUEEN = 5,
    KING = 6,
    WALL = 7
};

enum MoveFlag : int {
    QUIET = 0,
    CAPTURE = 1 << 0,
    PROMOTION = 1 << 1,
    PROMOTION_CAPTURE = 1 << 2,
    CASTLE = 1 << 3,
    EP_CAPTURE = 1 << 4,
    DOUBLE_PAWN_PUSH = 1 << 5
};

inline int teamOfColor(int color) {
    return (color == RED || color == YELLOW) ? TEAM_RY : TEAM_BG;
}

inline bool sameTeam(int c1, int c2) {
    return c1 != NO_COLOR && c2 != NO_COLOR && teamOfColor(c1) == teamOfColor(c2);
}

inline bool enemyColor(int c1, int c2) {
    return c1 != NO_COLOR && c2 != NO_COLOR && teamOfColor(c1) != teamOfColor(c2);
}

struct Move {
    uint16_t from;
    uint16_t to;

    uint8_t moved;
    uint8_t captured;
    uint8_t promotion;
    uint8_t flag;

    uint8_t movedColor;
    uint8_t capturedColor;

    Move()
        : from(0),
        to(0),
        moved(EMPTY),
        captured(EMPTY),
        promotion(EMPTY),
        flag(QUIET),
        movedColor(NO_COLOR),
        capturedColor(NO_COLOR) {
    }

    Move(
        int f,
        int t,
        int m,
        int cap,
        int promo,
        int flg,
        int mc,
        int cc
    )
        : from(static_cast<uint16_t>(f)),
        to(static_cast<uint16_t>(t)),
        moved(static_cast<uint8_t>(m)),
        captured(static_cast<uint8_t>(cap)),
        promotion(static_cast<uint8_t>(promo)),
        flag(static_cast<uint8_t>(flg)),
        movedColor(static_cast<uint8_t>(mc)),
        capturedColor(static_cast<uint8_t>(cc)) {
    }

    inline bool isNone() const {
        return from == 0 && to == 0;
    }

    inline bool isCapture() const {
        return (flag & (CAPTURE | PROMOTION_CAPTURE | EP_CAPTURE)) != 0;
    }

    inline bool isPromotion() const {
        return (flag & (PROMOTION | PROMOTION_CAPTURE)) != 0;
    }

    inline bool isTactical() const {
        return isCapture() || isPromotion();
    }

    inline bool isQuiet() const {
        return !isNone() && !isTactical();
    }

    inline bool isNormalQuiet() const {
        return !isNone() && flag == QUIET;
    }
};

struct MoveList {
    Move moves[MAX_MOVES];
    int count = 0;

    inline void clear() {
        count = 0;
    }

    inline void push(const Move& m) {
        moves[count++] = m;
    }

    inline Move& operator[](int i) {
        return moves[i];
    }

    inline const Move& operator[](int i) const {
        return moves[i];
    }
};

struct Position {
    uint64_t key;

    int board[BOARD_SIZE];
    int color[BOARD_SIZE];

    bool valid[BOARD_SIZE];

    int pieceList[5][MAX_PIECES_PER_COLOR];
    int pieceCount[5];

    int pieceIndex[BOARD_SIZE];

    bool castlingRights[5][2];

    int enPassantSq[5];

    int kingSq[5];

    int turn;

    int halfmoveClock;
    int ply;

    Position() {
        clear();
    }

    void clear() {
        std::memset(board, 0, sizeof(board));
        std::memset(color, 0, sizeof(color));
        std::memset(valid, 0, sizeof(valid));
        std::memset(pieceList, 0, sizeof(pieceList));
        std::memset(pieceCount, 0, sizeof(pieceCount));
        std::memset(pieceIndex, -1, sizeof(pieceIndex));
        std::memset(kingSq, 0, sizeof(kingSq));
        std::memset(enPassantSq, -1, sizeof(enPassantSq));
        std::memset(castlingRights, 0, sizeof(castlingRights));

        turn = RED;
        halfmoveClock = 0;
        ply = 0;
        key = 0ULL;
    }

    inline bool isValidSquare(int sq) const {
        return sq >= 0 && sq < BOARD_SIZE && valid[sq];
    }

    inline bool isEmpty(int sq) const {
        return board[sq] == EMPTY;
    }

    inline bool occupied(int sq) const {
        return board[sq] != EMPTY;
    }
};

inline constexpr int sqCR(int file, int rank) {
    return (BOARD_RANKS - rank) * MAILBOX_WIDTH + file;
}

struct CastleInfo {
    int kingFrom;
    int kingTo;
    int rookFrom;
    int rookTo;
};

inline bool getCastleInfo(int color, int side, CastleInfo& info) {
    switch (color) {
    case RED:
        info.kingFrom = sqCR(8, 1);
        if (side == CASTLE_KINGSIDE) {
            info.kingTo   = sqCR(10, 1);
            info.rookFrom = sqCR(11, 1);
            info.rookTo   = sqCR(9, 1);
        }
        else {
            info.kingTo   = sqCR(6, 1);
            info.rookFrom = sqCR(4, 1);
            info.rookTo   = sqCR(7, 1);
        }
        return true;

    case YELLOW:
        info.kingFrom = sqCR(7, 14);
        if (side == CASTLE_KINGSIDE) {
            info.kingTo   = sqCR(9, 14);
            info.rookFrom = sqCR(11, 14);
            info.rookTo   = sqCR(8, 14);
        }
        else {
            info.kingTo   = sqCR(5, 14);
            info.rookFrom = sqCR(4, 14);
            info.rookTo   = sqCR(6, 14);
        }
        return true;

    case BLUE:
        info.kingFrom = sqCR(1, 7);
        if (side == CASTLE_KINGSIDE) {
            info.kingTo   = sqCR(1, 9);
            info.rookFrom = sqCR(1, 11);
            info.rookTo   = sqCR(1, 8);
        }
        else {
            info.kingTo   = sqCR(1, 5);
            info.rookFrom = sqCR(1, 4);
            info.rookTo   = sqCR(1, 6);
        }
        return true;

    case GREEN:
        info.kingFrom = sqCR(14, 8);
        if (side == CASTLE_KINGSIDE) {
            info.kingTo   = sqCR(14, 10);
            info.rookFrom = sqCR(14, 11);
            info.rookTo   = sqCR(14, 9);
        }
        else {
            info.kingTo   = sqCR(14, 6);
            info.rookFrom = sqCR(14, 4);
            info.rookTo   = sqCR(14, 7);
        }
        return true;

    default:
        return false;
    }
}

inline int castleSideFromKingMove(int color, int from, int to) {
    CastleInfo ks;
    CastleInfo qs;

    if (getCastleInfo(color, CASTLE_KINGSIDE, ks) &&
        from == ks.kingFrom &&
        to == ks.kingTo) {
        return CASTLE_KINGSIDE;
    }

    if (getCastleInfo(color, CASTLE_QUEENSIDE, qs) &&
        from == qs.kingFrom &&
        to == qs.kingTo) {
        return CASTLE_QUEENSIDE;
    }

    return -1;
}

inline void setCastlingRight(Position& pos, int color, int side, bool value) {
    if (color < RED || color > GREEN || side < 0 || side > 1) {
        return;
    }

    if (pos.castlingRights[color][side] == value) {
        return;
    }

    pos.key ^= zobristCastle[color][side];
    pos.castlingRights[color][side] = value;
}

inline void clearCastlingRight(Position& pos, int color, int side) {
    setCastlingRight(pos, color, side, false);
}

inline void clearBothCastlingRights(Position& pos, int color) {
    clearCastlingRight(pos, color, CASTLE_KINGSIDE);
    clearCastlingRight(pos, color, CASTLE_QUEENSIDE);
}

struct History {
    int from;
    int to;

    int moved;
    int movedColor;

    int captured;
    int capturedColor;

    int promotion;
    int flag;

    int oldKingSq;
    int oldHalfmoveClock;
    int oldTurn;

    bool oldCastlingRights[5][2];

    int oldEnPassantSq[5];

    uint64_t oldKey;
};

inline void updateCastlingRightsAfterMove(Position& pos, const Move& m, const History& h) {
    if (m.moved == KING) {
        clearBothCastlingRights(pos, m.movedColor);
    }

    if (m.moved == ROOK) {
        for (int side = 0; side < 2; ++side) {
            CastleInfo ci;
            if (getCastleInfo(m.movedColor, side, ci) && m.from == ci.rookFrom) {
                clearCastlingRight(pos, m.movedColor, side);
            }
        }
    }

    if (h.captured == ROOK && h.capturedColor >= RED && h.capturedColor <= GREEN) {
        for (int side = 0; side < 2; ++side) {
            CastleInfo ci;
            if (getCastleInfo(h.capturedColor, side, ci) && h.to == ci.rookFrom) {
                clearCastlingRight(pos, h.capturedColor, side);
            }
        }
    }
}

uint64_t computeZobristKey(const Position& pos);

inline void addPiece(Position& pos, int sq, int piece, int color) {
    pos.key ^= zobristPiece[color][piece][sq];

    pos.board[sq] = piece;
    pos.color[sq] = color;

    int index = pos.pieceCount[color]++;
    pos.pieceList[color][index] = sq;
    pos.pieceIndex[sq] = index;

    if (piece == KING) {
        pos.kingSq[color] = sq;
    }
}

inline void removePiece(Position& pos, int sq) {
    int piece = pos.board[sq];
    int color = pos.color[sq];

    pos.key ^= zobristPiece[color][piece][sq];

    int index = pos.pieceIndex[sq];
    int lastIndex = --pos.pieceCount[color];
    int lastSq = pos.pieceList[color][lastIndex];

    pos.pieceList[color][index] = lastSq;
    pos.pieceIndex[lastSq] = index;

    pos.board[sq] = EMPTY;
    pos.color[sq] = NO_COLOR;
    pos.pieceIndex[sq] = -1;
}

inline void movePiece(Position& pos, int from, int to) {
    int piece = pos.board[from];
    int color = pos.color[from];

    pos.key ^= zobristPiece[color][piece][from];
    pos.key ^= zobristPiece[color][piece][to];

    int index = pos.pieceIndex[from];

    pos.board[to] = piece;
    pos.color[to] = color;
    pos.pieceIndex[to] = index;
    pos.pieceList[color][index] = to;

    pos.board[from] = EMPTY;
    pos.color[from] = NO_COLOR;
    pos.pieceIndex[from] = -1;

    if (piece == KING) {
        pos.kingSq[color] = to;
    }
}

struct PawnMoveInfo {
    int forward;
    int capLeft;
    int capRight;
};

extern PawnMoveInfo pawnInfo[5];

void initPawnInfo();
void initKnightInfo(const Position& pos);
void initKingInfo(const Position& pos);

inline History doMove(Position& pos, const Move& m) {
    History h;

    h.from = m.from;
    h.to = m.to;

    h.moved = m.moved;
    h.movedColor = m.movedColor;

    h.captured = pos.board[m.to];
    h.capturedColor = pos.color[m.to];

    h.promotion = m.promotion;
    h.flag = m.flag;

    h.oldKingSq = pos.kingSq[m.movedColor];
    h.oldHalfmoveClock = pos.halfmoveClock;
    h.oldTurn = pos.turn;
    h.oldKey = pos.key;

    std::memcpy(h.oldEnPassantSq, pos.enPassantSq, sizeof(pos.enPassantSq));

    std::memcpy(h.oldCastlingRights, pos.castlingRights, sizeof(pos.castlingRights));

    pos.key ^= zobristTurn[pos.turn];

    if (pos.enPassantSq[m.movedColor] != -1) {
        pos.key ^= zobristEp[pos.turn][pos.enPassantSq[m.movedColor]];
        pos.enPassantSq[m.movedColor] = -1;
    }

    if (m.flag & EP_CAPTURE) {
        const int capSq = m.from + pawnInfo[m.movedColor].forward;

        h.captured = pos.board[capSq];
        h.capturedColor = pos.color[capSq];

        if (h.captured != EMPTY) {
            removePiece(pos, capSq);
        }
    }
    else if (h.captured != EMPTY) {
        removePiece(pos, m.to);
    }

    if (m.flag & CASTLE) {
        const int side = castleSideFromKingMove(m.movedColor, m.from, m.to);

        CastleInfo ci;
        getCastleInfo(m.movedColor, side, ci);

        movePiece(pos, ci.kingFrom, ci.kingTo);
        movePiece(pos, ci.rookFrom, ci.rookTo);
    }
    else {
        movePiece(pos, m.from, m.to);
    }

    if (m.isPromotion()) {
        pos.key ^= zobristPiece[m.movedColor][PAWN][m.to];
        pos.key ^= zobristPiece[m.movedColor][m.promotion][m.to];

        pos.board[m.to] = m.promotion;
    }

    if (m.flag & DOUBLE_PAWN_PUSH) {
        const int skippedSq = m.from + pawnInfo[m.movedColor].forward;
        pos.key ^= zobristEp[pos.turn][skippedSq];
        pos.enPassantSq[m.movedColor] = skippedSq;
    }

    updateCastlingRightsAfterMove(pos, m, h);

    pos.turn++;

    if (pos.turn > GREEN) {
        pos.turn = RED;
    }

    pos.key ^= zobristTurn[pos.turn];

    pos.ply++;

    return h;
}

inline void undoMove(Position& pos, const Move& m, const History& h) {
    pos.turn = h.oldTurn;
    pos.halfmoveClock = h.oldHalfmoveClock;
    pos.kingSq[h.movedColor] = h.oldKingSq;
    pos.ply--;

    std::memcpy(pos.enPassantSq, h.oldEnPassantSq, sizeof(pos.enPassantSq));

    if (m.flag & CASTLE) {
        const int side = castleSideFromKingMove(h.movedColor, h.from, h.to);

        CastleInfo ci;
        getCastleInfo(h.movedColor, side, ci);

        movePiece(pos, ci.kingTo, ci.kingFrom);
        movePiece(pos, ci.rookTo, ci.rookFrom);
    }
    else {
        pos.board[m.to] = h.moved;
        movePiece(pos, m.to, m.from);
    }

    if (h.captured != EMPTY) {
        if (h.flag & EP_CAPTURE) {
            const int capSq = h.from + pawnInfo[h.movedColor].forward;
            addPiece(pos, capSq, h.captured, h.capturedColor);
        }
        else {
            addPiece(pos, h.to, h.captured, h.capturedColor);
        }
    }

    std::memcpy(pos.castlingRights, h.oldCastlingRights, sizeof(pos.castlingRights));

    pos.key = h.oldKey;
}

inline bool anyEnemyEpSquare(const Position& pos, int color, int dest, int& capturedColorOut) {
    capturedColorOut = NO_COLOR;

    for (int enemy = RED; enemy <= GREEN; ++enemy) {
        if (!enemyColor(color, enemy)) {
            continue;
        }

        if (pos.enPassantSq[enemy] == dest) {
            capturedColorOut = enemy;
            return true;
        }
    }

    return false;
}

inline void clearEnPassant(Position& pos) {
    for (int color = RED; color <= GREEN; ++color) {
        pos.enPassantSq[color] = -1;
    }
}

constexpr int baseMailbox[BOARD_SIZE] = {
    -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0, -1, -1, -1, -1,
    -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0, -1, -1, -1, -1,
    -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0, -1, -1, -1, -1,
    -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1,
    -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1,
    -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1,
    -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1,
    -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1,
    -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1,
    -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1,
    -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1,
    -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0, -1, -1, -1, -1,
    -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0, -1, -1, -1, -1,
    -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0, -1, -1, -1, -1,
};

inline bool isPawnStartSquare(int color, int sq) {
    if (sq < 0 || sq >= BOARD_SIZE) {
        return false;
    }

    if (baseMailbox[sq] == -1) {
        return false;
    }

    const int row = sq / MAILBOX_WIDTH;
    const int col = sq % MAILBOX_WIDTH;

    switch (color) {
    case RED:
        return row == 12;

    case YELLOW:
        return row == 1;

    case BLUE:
        return col == 2;

    case GREEN:
        return col == 13;

    default:
        return false;
    }
}

inline void initBoardValidity(Position& pos) {
    for (int sq = 0; sq < BOARD_SIZE; ++sq) {
        pos.valid[sq] = baseMailbox[sq] != -1;
    }
}

inline void addDoublePawnMove(
    MoveList& list,
    int from,
    int to,
    int color
) {
    list.push(Move(
        from,
        to,
        PAWN,
        EMPTY,
        EMPTY,
        DOUBLE_PAWN_PUSH,
        color,
        NO_COLOR
    ));
}

inline bool attacksFromSlider(
    const Position& pos,
    int from,
    int target,
    const int* dirs,
    int dirCount
) {
    for (int i = 0; i < dirCount; ++i) {
        int dir = dirs[i];
        int sq = from + dir;

        while (pos.isValidSquare(sq)) {
            if (sq == target) {
                return true;
            }

            if (pos.board[sq] != EMPTY) {
                break;
            }

            sq += dir;
        }
    }

    return false;
}

inline bool pieceAttacksSquare(
    const Position& pos,
    int from,
    int piece,
    int color,
    int target
) {
    switch (piece) {
    case KNIGHT:
        for (int offset : knightOffsets) {
            if (from + offset == target && pos.isValidSquare(target)) {
                return true;
            }
        }
        return false;

    case BISHOP:
        return attacksFromSlider(pos, from, target, bishopDirs, 4);

    case ROOK:
        return attacksFromSlider(pos, from, target, rookDirs, 4);

    case QUEEN:
        return attacksFromSlider(pos, from, target, queenDirs, 8);

    case KING:
        for (int offset : kingOffsets) {
            if (from + offset == target && pos.isValidSquare(target)) {
                return true;
            }
        }
        return false;

    case PAWN: {
        const PawnMoveInfo& p = pawnInfo[color];
        return from + p.capLeft == target || from + p.capRight == target;
    }

    default:
        return false;
    }
}

inline bool isSquareAttackedByTeam(
    const Position& pos,
    int target,
    int attackingTeam
) {
    for (int color = RED; color <= GREEN; ++color) {
        if (teamOfColor(color) != attackingTeam) {
            continue;
        }

        int count = pos.pieceCount[color];

        for (int i = 0; i < count; ++i) {
            int from = pos.pieceList[color][i];
            int piece = pos.board[from];

            if (pieceAttacksSquare(pos, from, piece, color, target)) {
                return true;
            }
        }
    }

    return false;
}

inline bool inCheck(const Position& pos, int color) {
    int king = pos.kingSq[color];
    int enemyTeam = teamOfColor(color) == TEAM_RY ? TEAM_BG : TEAM_RY;

    return isSquareAttackedByTeam(pos, king, enemyTeam);
}

inline void tryAddMove(
    const Position& pos,
    MoveList& list,
    int from,
    int to,
    int moved,
    int color
) {
    if (!pos.isValidSquare(to)) {
        return;
    }

    int targetPiece = pos.board[to];
    int targetColor = pos.color[to];

    if (targetPiece == EMPTY) {
        list.push(Move(
            from,
            to,
            moved,
            EMPTY,
            EMPTY,
            QUIET,
            color,
            NO_COLOR
        ));
        return;
    }

    if (sameTeam(color, targetColor)) {
        return;
    }

    list.push(Move(
        from,
        to,
        moved,
        targetPiece,
        EMPTY,
        CAPTURE,
        color,
        targetColor
    ));
}

inline bool squaresBetweenEmpty(const Position& pos, int from, int to) {
    const int diff = to - from;

    int step = 0;

    if (diff % MAILBOX_WIDTH == 0) {
        step = diff > 0 ? MAILBOX_WIDTH : -MAILBOX_WIDTH;
    }
    else {
        step = diff > 0 ? 1 : -1;
    }

    for (int sq = from + step; sq != to; sq += step) {
        if (!pos.isValidSquare(sq) || pos.board[sq] != EMPTY) {
            return false;
        }
    }

    return true;
}

inline bool canCastleSide(const Position& pos, int color, int side) {
    CastleInfo ci;

    if (!getCastleInfo(color, side, ci)) {
        return false;
    }

    if (!pos.castlingRights[color][side]) {
        return false;
    }

    if (pos.board[ci.kingFrom] != KING || pos.color[ci.kingFrom] != color) {
        return false;
    }

    if (pos.board[ci.rookFrom] != ROOK || pos.color[ci.rookFrom] != color) {
        return false;
    }

    if (!squaresBetweenEmpty(pos, ci.kingFrom, ci.rookFrom)) {
        return false;
    }

    const int enemyTeam = teamOfColor(color) == TEAM_RY ? TEAM_BG : TEAM_RY;

    if (isSquareAttackedByTeam(pos, ci.kingFrom, enemyTeam)) {
        return false;
    }

    const int kingStep = ci.kingTo > ci.kingFrom
        ? ((ci.kingTo - ci.kingFrom) % MAILBOX_WIDTH == 0 ? MAILBOX_WIDTH : 1)
        : ((ci.kingFrom - ci.kingTo) % MAILBOX_WIDTH == 0 ? -MAILBOX_WIDTH : -1);

    const int kingPassSq = ci.kingFrom + kingStep;

    if (isSquareAttackedByTeam(pos, kingPassSq, enemyTeam)) {
        return false;
    }

    if (isSquareAttackedByTeam(pos, ci.kingTo, enemyTeam)) {
        return false;
    }

    if (isSquareAttackedByTeam(pos, ci.rookTo, enemyTeam)) {
        return false;
    }

    return true;
}

inline void generateCastlingMoves(const Position& pos, MoveList& list, int from, int color) {
    for (int side = 0; side < 2; ++side) {
        CastleInfo ci;

        if (!getCastleInfo(color, side, ci)) {
            continue;
        }

        if (from != ci.kingFrom) {
            continue;
        }

        if (!canCastleSide(pos, color, side)) {
            continue;
        }

        list.push(Move(
            ci.kingFrom,
            ci.kingTo,
            KING,
            EMPTY,
            EMPTY,
            CASTLE,
            color,
            NO_COLOR
        ));
    }
}

inline void generateKnightMoves(
    const Position& pos,
    MoveList& list,
    int from,
    int color
) {
    for (int i = 0; i < knightMoveCount[from]; ++i) {
        int to = knightMoves[from][i];
        tryAddMove(pos, list, from, to, KNIGHT, color);
    }
}

inline void generateKingMoves(
    const Position& pos,
    MoveList& list,
    int from,
    int color
) {
    for (int i = 0; i < kingMoveCount[from]; ++i) {
        int to = kingMoves[from][i];
        tryAddMove(pos, list, from, to, KING, color);
    }

    generateCastlingMoves(pos, list, from, color);
}

inline void generateSliderMoves(
    const Position& pos,
    MoveList& list,
    int from,
    int color,
    int piece,
    const int* dirs,
    int dirCount
) {
    for (int i = 0; i < dirCount; ++i) {
        int dir = dirs[i];
        int to = from + dir;

        while (pos.isValidSquare(to)) {
            int targetPiece = pos.board[to];

            if (targetPiece == EMPTY) {
                list.push(Move(
                    from,
                    to,
                    piece,
                    EMPTY,
                    EMPTY,
                    QUIET,
                    color,
                    NO_COLOR
                ));
            } else {
                int targetColor = pos.color[to];

                if (!sameTeam(color, targetColor)) {
                    list.push(Move(
                        from,
                        to,
                        piece,
                        targetPiece,
                        EMPTY,
                        CAPTURE,
                        color,
                        targetColor
                    ));
                }

                break;
            }

            to += dir;
        }
    }
}

inline bool isPromotionSquare(int color, int sq) {
    if (sq < 0 || sq >= BOARD_SIZE) {
        return false;
    }

    if (baseMailbox[sq] == -1) {
        return false;
    }

    const int row = sq / MAILBOX_WIDTH;
    const int col = sq % MAILBOX_WIDTH;

    switch (color) {
    case RED:
        return row == RED_PROMO_ROW;

    case YELLOW:
        return row == YELLOW_PROMO_ROW;

    case BLUE:
        return col == BLUE_PROMO_COL;

    case GREEN:
        return col == GREEN_PROMO_COL;

    default:
        return false;
    }
}

inline void addPawnMove(
    const Position& pos,
    MoveList& list,
    int from,
    int to,
    int color,
    bool capture
) {
    if (isPromotionSquare(color, to)) {
        int flag = capture ? PROMOTION_CAPTURE : PROMOTION;

        list.push(Move(
            from,
            to,
            PAWN,
            capture ? pos.board[to] : EMPTY,
            QUEEN,
            flag,
            color,
            capture ? pos.color[to] : NO_COLOR
        ));
    } else {
        int flag = capture ? CAPTURE : QUIET;

        list.push(Move(
            from,
            to,
            PAWN,
            capture ? pos.board[to] : EMPTY,
            EMPTY,
            flag,
            color,
            capture ? pos.color[to] : NO_COLOR
        ));
    }
}

inline void generatePawnMoves(
    const Position& pos,
    MoveList& list,
    int from,
    int color
) {
    const PawnMoveInfo& p = pawnInfo[color];

    int one = from + p.forward;

    if (pos.isValidSquare(one) && pos.board[one] == EMPTY) {
        addPawnMove(pos, list, from, one, color, false);

        const int two = one + p.forward;

        if (isPawnStartSquare(color, from) &&
            pos.isValidSquare(two) &&
            pos.board[two] == EMPTY) {

            addDoublePawnMove(list, from, two, color);
        }
    }

    int caps[2] = {
        from + p.capLeft,
        from + p.capRight
    };

    for (int to : caps) {
        if (!pos.isValidSquare(to)) {
            continue;
        }

        int targetPiece = pos.board[to];

        if (targetPiece == EMPTY) {
            int epCapturedColor = NO_COLOR;

            if (anyEnemyEpSquare(pos, color, to, epCapturedColor)) {
                const int capSq = from + p.forward;

                if (pos.isValidSquare(capSq) &&
                    pos.board[capSq] == PAWN &&
                    pos.color[capSq] == epCapturedColor) {
                    list.push(Move(
                        from,
                        to,
                        PAWN,
                        PAWN,
                        EMPTY,
                        EP_CAPTURE,
                        color,
                        epCapturedColor
                    ));
                }
            }

            continue;
        }

        int targetColor = pos.color[to];

        if (sameTeam(color, targetColor)) {
            continue;
        }

        addPawnMove(pos, list, from, to, color, true);
    }
}

inline void generateMoves(const Position& pos, MoveList& list, int color) {
    list.clear();

    const int count = pos.pieceCount[color];

    for (int i = 0; i < count; ++i) {
        int from = pos.pieceList[color][i];
        int piece = pos.board[from];

        switch (piece) {
        case PAWN:
            generatePawnMoves(pos, list, from, color);
            break;

        case KNIGHT:
            generateKnightMoves(pos, list, from, color);
            break;

        case BISHOP:
            generateSliderMoves(pos, list, from, color, BISHOP, bishopDirs, 4);
            break;

        case ROOK:
            generateSliderMoves(pos, list, from, color, ROOK, rookDirs, 4);
            break;

        case QUEEN:
            generateSliderMoves(pos, list, from, color, QUEEN, queenDirs, 8);
            break;

        case KING:
            generateKingMoves(pos, list, from, color);
            break;

        default:
            break;
        }
    }
}

inline void generateLegalMoves(Position& pos, MoveList& legal, int color) {
    MoveList pseudo;
    generateMoves(pos, pseudo, color);

    legal.clear();

    for (int i = 0; i < pseudo.count; ++i) {
        const Move& m = pseudo[i];

        History h = doMove(pos, m);

        bool illegal = inCheck(pos, color);

        undoMove(pos, m, h);

        if (!illegal) {
            legal.push(m);
        }
    }
}

constexpr int pieceValue[7] = {
    0,
    100,
    300,
    400,
    500,
    1100,
    200000
};

constexpr int mobilityValue[7] = {
    0,
    1,
    4,
    3,
    2,
    1,
    0
};

inline bool mobilityTargetAllowed(const Position& pos, int fromColor, int to) {
    if (!pos.isValidSquare(to)) {
        return false;
    }

    int targetPiece = pos.board[to];

    if (targetPiece == EMPTY) {
        return true;
    }

    int targetColor = pos.color[to];

    return !sameTeam(fromColor, targetColor);
}

inline int countPawnMobility(const Position& pos, int from, int color) {
    const PawnMoveInfo& p = pawnInfo[color];

    int mobility = 0;

    int one = from + p.forward;
    if (pos.isValidSquare(one) && pos.board[one] == EMPTY) {
        mobility++;
    }

    int cap1 = from + p.capLeft;
    int cap2 = from + p.capRight;

    if (pos.isValidSquare(cap1) &&
        pos.board[cap1] != EMPTY &&
        !sameTeam(color, pos.color[cap1])) {
        mobility++;
    }

    if (pos.isValidSquare(cap2) &&
        pos.board[cap2] != EMPTY &&
        !sameTeam(color, pos.color[cap2])) {
        mobility++;
    }

    return mobility;
}

inline int countKnightMobility(const Position& pos, int from, int color) {
    int mobility = 0;

    for (int i = 0; i < knightMoveCount[from]; ++i) {
        int to = knightMoves[from][i];

        if (mobilityTargetAllowed(pos, color, to)) {
            mobility++;
        }
    }

    return mobility;
}

inline int countKingMobility(const Position& pos, int from, int color) {
    int mobility = 0;

    for (int i = 0; i < kingMoveCount[from]; ++i) {
        int to = kingMoves[from][i];

        if (mobilityTargetAllowed(pos, color, to)) {
            mobility++;
        }
    }

    return mobility;
}

inline int countSliderMobility(
    const Position& pos,
    int from,
    int color,
    const int* dirs,
    int dirCount
) {
    int mobility = 0;

    for (int i = 0; i < dirCount; ++i) {
        int dir = dirs[i];
        int sq = from + dir;

        while (pos.isValidSquare(sq)) {
            int targetPiece = pos.board[sq];

            if (targetPiece == EMPTY) {
                mobility++;
                sq += dir;
                continue;
            }

            int targetColor = pos.color[sq];

            if (!sameTeam(color, targetColor)) {
                mobility++;
            }

            break;
        }
    }

    return mobility;
}

inline int countPieceMobility(const Position& pos, int sq, int piece, int color) {
    switch (piece) {
    case PAWN:
        return countPawnMobility(pos, sq, color);

    case KNIGHT:
        return countKnightMobility(pos, sq, color);

    case BISHOP:
        return countSliderMobility(pos, sq, color, bishopDirs, 4);

    case ROOK:
        return countSliderMobility(pos, sq, color, rookDirs, 4);

    case QUEEN:
        return countSliderMobility(pos, sq, color, queenDirs, 8);

    case KING:
        return countKingMobility(pos, sq, color);

    default:
        return 0;
    }
}

inline Score lazyEvaluate(const Position& pos, int povColor) {
    const int povTeam = teamOfColor(povColor);

    Score materialScore = 0;
    Score mobilityScore = 0;

    int teamMobility[3] = { 0, 0, 0 };

    for (int color = RED; color <= GREEN; ++color) {
        const int team = teamOfColor(color);
        const int sign = team == povTeam ? 1 : -1;

        const int count = pos.pieceCount[color];

        for (int i = 0; i < count; ++i) {
            const int sq = pos.pieceList[color][i];
            const int piece = pos.board[sq];

            materialScore += sign * pieceValue[piece];

            const int mob = countPieceMobility(pos, sq, piece, color);
            const int mobValue = mobilityValue[piece];

            mobilityScore += sign * mob * mobValue;
            teamMobility[team] += mob;
        }
    }

    const int enemyTeam = povTeam == TEAM_RY ? TEAM_BG : TEAM_RY;
    const int teamMobilityDiff = teamMobility[povTeam] - teamMobility[enemyTeam];

    Score score = materialScore;
    score += mobilityScore;
    score += teamMobilityDiff;

    return score;
}

char pieceToChar(int piece, int color);
void printBoard(const Position& pos);

void printMove(const Move& move);
void printMoveDetailed(const Move& move);