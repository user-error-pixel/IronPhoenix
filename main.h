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

constexpr Score INF = 30000000;
constexpr Score MATE_SCORE = 29000000;

constexpr int BOARD_WIDTH = 16;
constexpr int BOARD_RANKS = 14;

extern uint64_t zobristPiece[5][7][BOARD_SIZE];
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
    EP_CAPTURE = 1 << 4
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

    Move() = default;

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

    inline bool isCapture() const {
        return flag & (CAPTURE | PROMOTION_CAPTURE | EP_CAPTURE);
    }

    inline bool isPromotion() const {
        return flag & (PROMOTION | PROMOTION_CAPTURE);
    }

    inline bool isQuiet() const {
        return flag == QUIET;
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

    uint64_t oldKey;
};

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

    pos.key ^= zobristTurn[pos.turn];

    if (h.captured != EMPTY) {
        removePiece(pos, m.to);
    }

    movePiece(pos, m.from, m.to);

    if (m.isPromotion()) {
        pos.board[m.to] = m.promotion;
    }

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

    pos.board[m.to] = h.moved;

    movePiece(pos, m.to, m.from);

    if (h.captured != EMPTY) {
        addPiece(pos, m.to, h.captured, h.capturedColor);
    }

    pos.key = h.oldKey;
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

inline void initBoardValidity(Position& pos) {
    for (int sq = 0; sq < BOARD_SIZE; ++sq) {
        pos.valid[sq] = baseMailbox[sq] != -1;
    }
}

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

constexpr int knightOffsets[8] = {
    -18, -33,
    -31, -14,
     18,  33,
     31,  14
};

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

constexpr int kingOffsets[8] = {
    DIR_N, DIR_S, DIR_E, DIR_W,
    DIR_NE, DIR_NW, DIR_SE, DIR_SW
};

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

struct PawnMoveInfo {
    int forward;
    int capLeft;
    int capRight;
};

extern PawnMoveInfo pawnInfo[5];

void initPawnInfo();
void initKnightInfo(const Position& pos);
void initKingInfo(const Position& pos);

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
