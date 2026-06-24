#pragma once
#include <atomic>
#include <string>
#include <cstdint>
#include "main.h"
#include "tt.h"

constexpr int MAX_PLY = 128;

constexpr Score ASPIRATION_INITIAL_WINDOW = 50;
constexpr Score ASPIRATION_MAX_WINDOW = 2000;

constexpr Depth NMP_MIN_DEPTH = 3;
constexpr Depth NMP_BASE_REDUCTION = 3;
constexpr Score NMP_EVAL_MARGIN = 50;
constexpr Score NMP_MAX_EVAL_BONUS = 800;

constexpr int MAX_SEARCH_PLY = 128;
constexpr int KILLER_SLOTS = 2;

constexpr int RFP_MARGIN = 150;
constexpr int RFP_MAX_DEPTH = 3;

constexpr int MAX_LMR_DEPTH = MAX_PLY + 8;
constexpr int MAX_LMR_MOVES = MAX_MOVES;

constexpr Score MATE_FOUND_THRESHOLD = MATE_SCORE - MAX_PLY;

extern int lmrReductions[MAX_LMR_DEPTH][MAX_LMR_MOVES + 1];

inline bool isMateScore(Score score) {
    return score >= MATE_FOUND_THRESHOLD || score <= -MATE_FOUND_THRESHOLD;
}

inline int mateDistancePly(Score score) {
    if (score > 0) {
        return MATE_SCORE - score;
    }

    return MATE_SCORE + score;
}

inline int mateDistanceMoves(Score score) {
    int plies = std::abs(mateDistancePly(score));

    return (plies + 1) / 4;
}

inline int signedMateDistanceMoves(Score score) {
    int moves = mateDistanceMoves(score);

    if (score < 0) {
        return -moves;
    }

    return moves;
}

inline bool isWinningMateScore(Score score) {
    return score >= MATE_FOUND_THRESHOLD;
}

inline bool isLosingMateScore(Score score) {
    return score <= -MATE_FOUND_THRESHOLD;
}

void initLmrReductions();

inline Depth getBaseLmrReduction(Depth depth, int movesSearched) {
    if (depth < 0) {
        depth = 0;
    }

    if (depth >= MAX_LMR_DEPTH) {
        depth = MAX_LMR_DEPTH - 1;
    }

    if (movesSearched < 0) {
        movesSearched = 0;
    }

    if (movesSearched > MAX_LMR_MOVES) {
        movesSearched = MAX_LMR_MOVES;
    }

    return lmrReductions[depth][movesSearched];
}

struct SearchStack {
    Move currentMove;
    Move excludedMove;

    Score staticEval = 0;
    Score previousStaticEval = 0;

    bool inCheck = false;
    bool improving = false;
    bool ttPv = false;
    bool nullMove = true;

    int ply = 0;
    int moveCount = 0;

    void clear() {
        currentMove = Move();
        excludedMove = Move();

        staticEval = 0;
        previousStaticEval = 0;

        inCheck = false;
        improving = false;
        ttPv = false;
        nullMove = true;

        ply = 0;
        moveCount = 0;
    }
};

struct PVLine {
    Move moves[MAX_PLY];
    int length = 0;

    inline void clear() {
        length = 0;
    }
};

struct SearchStats {
    uint64_t nodes = 0;
    uint64_t qnodes = 0;
    uint64_t ttHits = 0;
    uint64_t betaCutoffs = 0;

    int seldepth = 0;

    void clear() {
        nodes = 0;
        qnodes = 0;
        ttHits = 0;
        betaCutoffs = 0;
        seldepth = 0;
    }
};

class Search {
public:
    explicit Search(TranspositionTable& table);

    Move findBestMove(Position& pos, Depth maxDepth, int movetimeMs = 0);

    Score negamax(
        Position& pos,
        SearchStack* ss,
        Depth depth,
        Score alpha,
        Score beta,
        bool isPv,
        PVLine& pv
    );

    Score qsearch(
        Position& pos,
        Score alpha,
        Score beta
    );

    void stop();

    SearchStats stats;

    std::string pvToString(const PVLine& pv) const;

private:
    TranspositionTable& tt;

    int quietHistory[5][BOARD_SIZE][BOARD_SIZE];

    int captureHistory[7][BOARD_SIZE][7];

    void clearHistory();

    void updateQuietHistory(const Move& move, int bonus);
    void updateCaptureHistory(const Move& move, int bonus);

    int getQuietHistory(const Move& move) const;
    int getCaptureHistory(const Move& move) const;

    int historyBonus(Depth depth) const;

    Move killerMoves[MAX_SEARCH_PLY][KILLER_SLOTS];

    void clearKillers();

    void storeKiller(int ply, const Move& move);

    bool isKillerMove(int ply, const Move& move) const;

    bool canReverseFutilityPrune(
        const SearchStack* ss,
        Depth depth,
        Score beta,
        bool isPv
    ) const;

    bool canNullMovePrune(
        const SearchStack* ss,
        Depth depth,
        Score beta,
        bool isPv
    ) const;

    Depth nullMoveReduction(
        const SearchStack* ss,
        Depth depth,
        Score beta
    ) const;

    Depth lmrReduction(
        const SearchStack* ss,
        Depth depth,
        int moveCount,
        bool isPv,
        const Move& move
    ) const;

    bool stopSearch = false;
    bool useTimeLimit = false;

    std::chrono::steady_clock::time_point searchStartTime;
    int timeLimitMs = 0;

    bool shouldStop();
    void startTimer(int movetimeMs);

    void updateSelDepth(const Position& pos);

    Move rootBestMove;

    int scoreMove(
        const Position& pos,
        const Move& move,
        const Move& ttMove
    ) const;

    void orderMoves(
        const Position& pos,
        MoveList& moves,
        const Move& ttMove
    ) const;

    bool sameMove(const Move& a, const Move& b) const;

    Score terminalScore(const Position& pos, int color) const;
};