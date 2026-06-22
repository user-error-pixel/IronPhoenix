#pragma once
#include <atomic>
#include <string>
#include <cstdint>
#include "main.h"
#include "tt.h"

constexpr int MAX_PLY = 128;

constexpr int MAX_SEARCH_PLY = 128;
constexpr int KILLER_SLOTS = 2;

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