#pragma once
#include <cstdint>
#include <vector>
#include <cstddef>
#include "main.h"

enum TTBound : uint8_t {
    TT_NONE = 0,
    TT_EXACT = 1,
    TT_LOWER = 2,
    TT_UPPER = 3
};

constexpr uint8_t TT_BOUND_MASK = 0b00000011;

constexpr uint8_t TT_PV_FLAG = 0b00000100;

struct TTEntry {
    uint16_t key16;
    int16_t score;
    int16_t eval;
    int8_t depth;
    uint8_t flags;
    uint8_t age;

    uint8_t moveFrom;
    uint8_t moveTo;
    uint8_t movePromo;
    uint8_t moveFlags;

    TTEntry();

    inline TTBound bound() const {
        return static_cast<TTBound>(flags & TT_BOUND_MASK);
    }

    inline bool ttPv() const {
        return (flags & TT_PV_FLAG) != 0;
    }

    inline bool occupied() const {
        return bound() != TT_NONE;
    }
};

static_assert(sizeof(TTEntry) <= 16, "TTEntry should stay compact.");

struct TTCluster {
    TTEntry entry[4];
};

class TranspositionTable {
public:
    TranspositionTable();

    void resizeMB(std::size_t mb);
    void clear();
    void newSearch();

    bool probe(
        const Position& pos,
        Depth depth,
        Score alpha,
        Score beta,
        Score& outScore,
        Move& outMove,
        Score& outEval,
        bool& outTtPv
    );

    void store(
        const Position& pos,
        Depth depth,
        Score score,
        Score eval,
        TTBound bound,
        const Move& bestMove,
        bool isPv
    );

    std::size_t sizeMB() const;
    std::size_t clusters() const;

private:
    std::vector<TTCluster> table;
    std::size_t clusterCount;
    uint8_t currentAge;

    inline uint16_t makeKey16(uint64_t key) const {
        return static_cast<uint16_t>(key >> 48);
    }

    inline TTCluster& getCluster(uint64_t key) {
        return table[key & (clusterCount - 1)];
    }

    Move makeStoredMove(const TTEntry& entry, const Position& pos) const;
};