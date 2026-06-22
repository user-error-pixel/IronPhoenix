#include "tt.h"

#include <cstring>
#include <algorithm>

TTEntry::TTEntry()
    : key16(0),
    score(0),
    eval(0),
    depth(-1),
    flags(TT_NONE),
    age(0),
    moveFrom(0),
    moveTo(0),
    movePromo(0),
    moveFlags(0) {
}

TranspositionTable::TranspositionTable()
    : clusterCount(0),
    currentAge(0) {
}

void TranspositionTable::resizeMB(std::size_t mb) {
    const std::size_t bytes = mb * 1024ULL * 1024ULL;

    clusterCount = 1;

    while ((clusterCount << 1) * sizeof(TTCluster) <= bytes) {
        clusterCount <<= 1;
    }

    table.clear();
    table.resize(clusterCount);

    clear();
}

void TranspositionTable::clear() {
    if (!table.empty()) {
        std::memset(table.data(), 0, table.size() * sizeof(TTCluster));
    }

    currentAge = 0;
}

void TranspositionTable::newSearch() {
    ++currentAge;
}

std::size_t TranspositionTable::sizeMB() const {
    return (table.size() * sizeof(TTCluster)) / (1024ULL * 1024ULL);
}

std::size_t TranspositionTable::clusters() const {
    return clusterCount;
}

Move TranspositionTable::makeStoredMove(const TTEntry& entry, const Position& pos) const {
    const int from = entry.moveFrom;
    const int to = entry.moveTo;

    if (from < 0 || from >= BOARD_SIZE || to < 0 || to >= BOARD_SIZE) {
        return Move();
    }

    const int moved = pos.board[from];
    const int movedColor = pos.color[from];

    if (moved == EMPTY || movedColor == NO_COLOR) {
        return Move();
    }

    const int captured = pos.board[to];
    const int capturedColor = pos.color[to];

    return Move(
        from,
        to,
        moved,
        captured,
        entry.movePromo,
        entry.moveFlags,
        movedColor,
        capturedColor
    );
}

bool TranspositionTable::probe(
    const Position& pos,
    Depth depth,
    Score alpha,
    Score beta,
    Score& outScore,
    Move& outMove,
    Score& outEval,
    bool& outTtPv
) {
    if (clusterCount == 0 || table.empty()) {
        return false;
    }

    const uint64_t key = pos.key;
    const uint16_t k16 = makeKey16(key);

    TTCluster& cluster = getCluster(key);

    for (int i = 0; i < 4; ++i) {
        TTEntry& entry = cluster.entry[i];

        if (entry.key16 != k16 || !entry.occupied()) {
            continue;
        }

        entry.age = currentAge;

        outMove = makeStoredMove(entry, pos);
        outEval = static_cast<Score>(entry.eval);
        outTtPv = entry.ttPv();

        if (entry.depth >= depth) {
            const Score score = static_cast<Score>(entry.score);
            const TTBound bound = entry.bound();

            if (bound == TT_EXACT) {
                outScore = score;
                return true;
            }

            if (bound == TT_LOWER && score >= beta) {
                outScore = score;
                return true;
            }

            if (bound == TT_UPPER && score <= alpha) {
                outScore = score;
                return true;
            }
        }

        return false;
    }

    return false;
}

void TranspositionTable::store(
    const Position& pos,
    Depth depth,
    Score score,
    Score eval,
    TTBound bound,
    const Move& bestMove,
    bool isPv
) {
    if (clusterCount == 0 || table.empty()) {
        return;
    }

    const uint64_t key = pos.key;
    const uint16_t k16 = makeKey16(key);

    TTCluster& cluster = getCluster(key);

    TTEntry* replace = &cluster.entry[0];

    for (int i = 0; i < 4; ++i) {
        TTEntry& entry = cluster.entry[i];

        if (entry.key16 == k16) {
            replace = &entry;
            break;
        }

        if (!entry.occupied()) {
            replace = &entry;
            break;
        }

        const int entryReplaceScore =
            (entry.age == currentAge ? 0 : 32)
            + (static_cast<int>(depth) - static_cast<int>(entry.depth)) * 2
            + (entry.bound() == TT_EXACT ? 0 : 4)
            + (entry.ttPv() ? -2 : 0);

        const int bestReplaceScore =
            (replace->age == currentAge ? 0 : 32)
            + (static_cast<int>(depth) - static_cast<int>(replace->depth)) * 2
            + (replace->bound() == TT_EXACT ? 0 : 4)
            + (replace->ttPv() ? -2 : 0);

        if (entryReplaceScore > bestReplaceScore) {
            replace = &entry;
        }
    }

    if (replace->key16 == k16 &&
        replace->bound() == TT_EXACT &&
        replace->depth > depth &&
        bound != TT_EXACT) {
        return;
    }

    uint8_t packedFlags = static_cast<uint8_t>(bound & TT_BOUND_MASK);

    if (isPv) {
        packedFlags |= TT_PV_FLAG;
    }

    replace->key16 = k16;
    replace->score = static_cast<int16_t>(std::clamp(score, -32760, 32760));
    replace->eval = static_cast<int16_t>(std::clamp(eval, -32760, 32760));
    replace->depth = static_cast<int8_t>(std::clamp(static_cast<int>(depth), -128, 127));
    replace->flags = packedFlags;
    replace->age = currentAge;

    replace->moveFrom = static_cast<uint8_t>(bestMove.from);
    replace->moveTo = static_cast<uint8_t>(bestMove.to);
    replace->movePromo = static_cast<uint8_t>(bestMove.promotion);
    replace->moveFlags = static_cast<uint8_t>(bestMove.flag);
}