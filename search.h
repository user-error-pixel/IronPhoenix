#pragma once
#include <string>
#include <cstdint>
#include "main.h"
#include "tt.h"

constexpr int MAX_PLY = 128;

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

    void clear() {
        nodes = 0;
        qnodes = 0;
        ttHits = 0;
        betaCutoffs = 0;
    }
};

class Search {
public:
    explicit Search(TranspositionTable& table);

    Move findBestMove(Position& pos, Depth depth);

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

    SearchStats stats;

    std::string pvToString(const PVLine& pv) const;

private:
    TranspositionTable& tt;

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