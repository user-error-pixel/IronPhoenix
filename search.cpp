#include <algorithm>
#include <iostream>
#include <chrono>
#include "search.h"
#include "preft.h"

std::atomic<bool> stopSearch = false;

Search::Search(TranspositionTable& table)
    : tt(table),
    rootBestMove() {
    clearKillers();
    clearHistory();
}

bool Search::sameMove(const Move& a, const Move& b) const {
    if (a.from == 0 && a.to == 0) {
        return false;
    }

    if (b.from == 0 && b.to == 0) {
        return false;
    }

    return a.from == b.from
        && a.to == b.to
        && a.promotion == b.promotion
        && a.flag == b.flag;
}

std::string Search::pvToString(const PVLine& pv) const {
    std::string out;

    for (int i = 0; i < pv.length; ++i) {
        if (i > 0) {
            out += " ";
        }

        out += moveToUci(pv.moves[i]);
    }

    return out;
}

void Search::clearHistory() {
    std::memset(quietHistory, 0, sizeof(quietHistory));
    std::memset(captureHistory, 0, sizeof(captureHistory));
}

int Search::historyBonus(Depth depth) const {
    int bonus = depth * depth;

    if (bonus > 400) {
        bonus = 400;
    }

    return bonus;
}

void Search::updateQuietHistory(const Move& move, int bonus) {
    if (!move.isQuiet()) {
        return;
    }

    int& entry = quietHistory[move.movedColor][move.from][move.to];

    entry += bonus;

    if (entry > 32000) {
        entry = 32000;
    }
}

void Search::updateCaptureHistory(const Move& move, int bonus) {
    if (!move.isCapture()) {
        return;
    }

    int moved = move.moved;
    int captured = move.captured;
    int to = move.to;

    if (moved <= EMPTY || moved > KING || captured <= EMPTY || captured > KING) {
        return;
    }

    int& entry = captureHistory[moved][to][captured];

    entry += bonus;

    if (entry > 32000) {
        entry = 32000;
    }
}

int Search::getQuietHistory(const Move& move) const {
    if (!move.isQuiet()) {
        return 0;
    }

    return quietHistory[move.movedColor][move.from][move.to];
}

int Search::getCaptureHistory(const Move& move) const {
    if (!move.isCapture()) {
        return 0;
    }

    int moved = move.moved;
    int captured = move.captured;
    int to = move.to;

    if (moved <= EMPTY || moved > KING || captured <= EMPTY || captured > KING) {
        return 0;
    }

    return captureHistory[moved][to][captured];
}

void Search::clearKillers() {
    for (int ply = 0; ply < MAX_SEARCH_PLY; ++ply) {
        for (int slot = 0; slot < KILLER_SLOTS; ++slot) {
            killerMoves[ply][slot] = Move();
        }
    }
}

void Search::storeKiller(int ply, const Move& move) {
    if (ply < 0 || ply >= MAX_SEARCH_PLY) {
        return;
    }

    if (!move.isQuiet()) {
        return;
    }

    if (sameMove(killerMoves[ply][0], move)) {
        return;
    }

    killerMoves[ply][1] = killerMoves[ply][0];
    killerMoves[ply][0] = move;
}

bool Search::isKillerMove(int ply, const Move& move) const {
    if (ply < 0 || ply >= MAX_SEARCH_PLY) {
        return false;
    }

    return sameMove(killerMoves[ply][0], move)
        || sameMove(killerMoves[ply][1], move);
}

void Search::updateSelDepth(const Position& pos) {
    if (pos.ply > stats.seldepth) {
        stats.seldepth = pos.ply;
    }
}

void Search::stop() {
    stopSearch = true;
}

void Search::startTimer(int movetimeMs) {
    stopSearch = false;
    timeLimitMs = movetimeMs;
    useTimeLimit = movetimeMs > 0;
    searchStartTime = std::chrono::steady_clock::now();
}

bool Search::shouldStop() {
    if (!useTimeLimit) {
        return false;
    }

    if ((stats.nodes & 2047ULL) != 0ULL) {
        return false;
    }

    const auto now = std::chrono::steady_clock::now();
    const int elapsedMs = static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now - searchStartTime
        ).count()
        );

    if (elapsedMs >= timeLimitMs) {
        stopSearch = true;
        return true;
    }

    return false;
}

Score Search::terminalScore(const Position& pos, int color) const {
    if (inCheck(pos, color)) {
        return -MATE_SCORE + pos.ply;
    }

    return 0;
}

int Search::scoreMove(
    const Position& pos,
    const Move& move,
    const Move& ttMove
) const {
    int score = 0;

    if (sameMove(move, ttMove)) {
        return 2'000'000'000;
    }

    if (move.isCapture()) {
        score += 1'000'000;
        score += pieceValue[move.captured] * 16;
        score -= pieceValue[move.moved];

        score += getCaptureHistory(move);
    }

    if (move.isPromotion()) {
        score += 900'000;
        score += pieceValue[move.promotion];
    }

    if (move.isQuiet() && isKillerMove(pos.ply, move)) {
        if (sameMove(killerMoves[pos.ply][0], move)) {
            score += 800'000;
        }
        else {
            score += 700'000;
        }
    }

    score += getQuietHistory(move);

    return score;
}

void Search::orderMoves(
    const Position& pos,
    MoveList& moves,
    const Move& ttMove
) const {
    for (int i = 0; i < moves.count - 1; ++i) {
        int bestIndex = i;
        int bestScore = scoreMove(pos, moves[i], ttMove);

        for (int j = i + 1; j < moves.count; ++j) {
            int s = scoreMove(pos, moves[j], ttMove);

            if (s > bestScore) {
                bestScore = s;
                bestIndex = j;
            }
        }

        if (bestIndex != i) {
            std::swap(moves.moves[i], moves.moves[bestIndex]);
        }
    }
}

Score Search::qsearch(
    Position& pos,
    Score alpha,
    Score beta
) {
    stats.qnodes++;

    if (shouldStop()) {
        return lazyEvaluate(pos, pos.turn);
    }

    if (stopSearch) {
        return lazyEvaluate(pos, pos.turn);
    }

    Score standPat = lazyEvaluate(pos, pos.turn);

    if (standPat >= beta) {
        return beta;
    }

    if (standPat > alpha) {
        alpha = standPat;
    }

    MoveList moves;
    generateMoves(pos, moves, pos.turn);

    for (int i = 0; i < moves.count; ++i) {
        if (stopSearch) {
            break;
        }

        const Move& move = moves[i];

        if (!move.isCapture() && !move.isPromotion()) {
            continue;
        }

        const int movingColor = pos.turn;

        History h = doMove(pos, move);

        if (inCheck(pos, movingColor)) {
            undoMove(pos, move, h);
            continue;
        }

        Score score = -qsearch(pos, -beta, -alpha);

        undoMove(pos, move, h);

        if (score >= beta) {
            return beta;
        }

        if (score > alpha) {
            alpha = score;
        }
    }

    return alpha;
}

Score Search::negamax(
    Position& pos,
    Depth depth,
    Score alpha,
    Score beta,
    bool isPv,
    PVLine& pv
) {
    stats.nodes++;
    pv.clear();

    updateSelDepth(pos);

    if (shouldStop()) {
        return lazyEvaluate(pos, pos.turn);
    }

    if (stopSearch) {
        return lazyEvaluate(pos, pos.turn);
    }

    const Score oldAlpha = alpha;

    if (depth <= 0) {
        return qsearch(pos, alpha, beta);
    }

    Move ttMove;
    Score ttScore = 0;
    Score ttEval = 0;
    bool ttPv = false;

    if (tt.probe(pos, depth, alpha, beta, ttScore, ttMove, ttEval, ttPv)) {
        stats.ttHits++;
        return ttScore;
    }

    MoveList moves;
    generateLegalMoves(pos, moves, pos.turn);

    if (moves.count == 0) {
        return terminalScore(pos, pos.turn);
    }

    orderMoves(pos, moves, ttMove);

    Move bestMove = moves[0];
    Score bestScore = -INF;

    for (int i = 0; i < moves.count; ++i) {
        if (stopSearch) {
            break;
        }

        const Move& move = moves[i];

        PVLine childPv;

        History h = doMove(pos, move);

        Score score = -negamax(
            pos,
            depth - 1,
            -beta,
            -alpha,
            isPv && i == 0,
            childPv
        );

        undoMove(pos, move, h);

        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }

        if (score > alpha) {
            alpha = score;

            pv.length = 1;
            pv.moves[0] = move;

            const int copyCount = std::min(childPv.length, MAX_PLY - 1);

            for (int j = 0; j < copyCount; ++j) {
                pv.moves[j + 1] = childPv.moves[j];
            }

            pv.length += copyCount;
        }
        
        if (alpha >= beta) {
            stats.betaCutoffs++;

            const int bonus = historyBonus(depth);

            if (move.isQuiet()) {
                storeKiller(pos.ply, move);
                updateQuietHistory(move, bonus);
            }
            else if (move.isCapture()) {
                updateCaptureHistory(move, bonus);
            }

            break;
        }
    }

    TTBound bound = TT_EXACT;

    if (bestScore <= oldAlpha) {
        bound = TT_UPPER;
    }
    else if (bestScore >= beta) {
        bound = TT_LOWER;
    }

    Score eval = lazyEvaluate(pos, pos.turn);

    tt.store(
        pos,
        depth,
        bestScore,
        eval,
        bound,
        bestMove,
        isPv
    );

    return bestScore;
}

Move Search::findBestMove(Position& pos, Depth maxDepth, int movetimeMs) {
    stats.clear();
    clearKillers();
    tt.newSearch();
    startTimer(movetimeMs);

    Move bestMove{};
    PVLine bestPv;
    Score bestScore = -INF;

    for (Depth depth = 1; depth <= maxDepth; ++depth) {
        if (stopSearch) {
            break;
        }

        stats.seldepth = 0;

        MoveList moves;
        generateLegalMoves(pos, moves, pos.turn);

        if (moves.count == 0) {
            return Move();
        }

        Move ttMove;
        Score ttScore = 0;
        Score ttEval = 0;
        bool ttPv = false;

        tt.probe(pos, depth, -INF, INF, ttScore, ttMove, ttEval, ttPv);

        orderMoves(pos, moves, ttMove);

        Move depthBestMove = moves[0];

        Score alpha = -INF;
        Score beta = INF;
        Score depthBestScore = -INF;

        PVLine rootPv;
        rootPv.clear();

        for (int i = 0; i < moves.count; ++i) {
            if (stopSearch) {
                break;
            }

            const Move& move = moves[i];

            PVLine childPv;

            History h = doMove(pos, move);

            Score score = -negamax(
                pos,
                depth - 1,
                -beta,
                -alpha,
                i == 0,
                childPv
            );

            undoMove(pos, move, h);

            if (stopSearch) {
                break;
            }

            if (score > depthBestScore) {
                depthBestScore = score;
                depthBestMove = move;
            }

            if (score > alpha) {
                alpha = score;

                rootPv.length = 1;
                rootPv.moves[0] = move;

                const int copyCount = std::min(childPv.length, MAX_PLY - 1);

                for (int j = 0; j < copyCount; ++j) {
                    rootPv.moves[j + 1] = childPv.moves[j];
                }

                rootPv.length += copyCount;
            }
        }

        if (stopSearch) {
            break;
        }

        Score eval = lazyEvaluate(pos, pos.turn);

        tt.store(
            pos,
            depth,
            depthBestScore,
            eval,
            TT_EXACT,
            depthBestMove,
            true
        );

        bestMove = depthBestMove;
        bestScore = depthBestScore;
        bestPv = rootPv;

        const auto now = std::chrono::steady_clock::now();
        const double elapsedSeconds =
            std::chrono::duration<double>(now - searchStartTime).count();

        const uint64_t totalNodes = stats.nodes + stats.qnodes;

        const uint64_t nps = elapsedSeconds > 0.0
            ? static_cast<uint64_t>(static_cast<double>(totalNodes) / elapsedSeconds)
            : 0ULL;

        const uint64_t timeMs =
            static_cast<uint64_t>(elapsedSeconds * 1000.0);

        std::cout << "info depth " << depth
            << " seldepth " << stats.seldepth;

        if (isMateScore(bestScore)) {
            std::cout << " score cp mate " << signedMateDistanceMoves(bestScore);
        }
        else {
            std::cout << " score cp " << bestScore;
        }

        std::cout
            << " nodes " << stats.nodes
            << " qnodes " << stats.qnodes
            << " time " << timeMs
            << " nps " << nps
            << " pv " << pvToString(bestPv)
            << "\n";

        if (isMateScore(bestScore)) {
            stopSearch = true;
            break;
        }
    }

    rootBestMove = bestMove;
    return bestMove;
}