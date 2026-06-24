#include <algorithm>
#include <iostream>
#include <chrono>
#include <cmath>
#include "search.h"
#include "preft.h"

std::atomic<bool> stopSearch = false;

SearchStack stack[MAX_PLY + 8];

int lmrReductions[MAX_LMR_DEPTH][MAX_LMR_MOVES + 1];

Search::Search(TranspositionTable& table)
    : tt(table),
    rootBestMove() {
    clearKillers();
    clearHistory();
    initLmrReductions();
}

void initLmrReductions() {
    for (int depth = 0; depth < MAX_LMR_DEPTH; ++depth) {
        for (int movesSearched = 0; movesSearched <= MAX_LMR_MOVES; ++movesSearched) {
            int reduction = 0;

            if (depth >= 3 && movesSearched >= 4) {
                const double value =
                    0.99 +
                    (std::log(static_cast<double>(depth)) *
                     std::log(static_cast<double>(movesSearched))) / 3.14;

                reduction = static_cast<int>(value);

                if (reduction > depth - 1) {
                    reduction = depth - 1;
                }

                if (reduction < 0) {
                    reduction = 0;
                }
            }

            lmrReductions[depth][movesSearched] = reduction;
        }
    }
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

Depth Search::lmrReduction(
    const SearchStack* ss,
    Depth depth,
    int moveCount,
    bool isPv,
    const Move& move
) const {
    if (depth < 3) {
        return 0;
    }

    if (moveCount < 4) {
        return 0;
    }

    if (!move.isQuiet()) {
        return 0;
    }

    if (ss->inCheck) {
        return 0;
    }

    if (move.flag & CASTLE) {
        return 0;
    }

    Depth reduction = getBaseLmrReduction(depth, moveCount);

    reduction = std::clamp<Depth>(reduction, 0, depth - 1);

    return reduction;
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

bool Search::canReverseFutilityPrune(
    const SearchStack* ss,
    Depth depth,
    Score beta,
    bool isPv
) const {
    if (isPv) {
        return false;
    }

    if (ss->inCheck) {
        return false;
    }
    
    if (depth <= 0 || depth > RFP_MAX_DEPTH) {
        return false;
    }

    if (std::abs(beta) > MATE_SCORE - 1000) {
        return false;
    }

    if (std::abs(ss->staticEval) > MATE_SCORE - 1000) {
        return false;
    }

    int margin = RFP_MARGIN * depth;

    if (ss->improving) {
        margin -= 50;
    }

    return ss->staticEval >= beta + margin;
}

bool Search::canNullMovePrune(
    const SearchStack* ss,
    Depth depth,
    Score beta,
    bool isPv
) const {
    if (isPv) {
        return false;
    }

    if (ss->inCheck) {
        return false;
    }

    if (depth < NMP_MIN_DEPTH) {
        return false;
    }

    if (std::abs(beta) > MATE_SCORE - 1000) {
        return false;
    }

    if (std::abs(ss->staticEval) > MATE_SCORE - 1000) {
        return false;
    }

    if (ss->staticEval < beta + NMP_EVAL_MARGIN) {
        return false;
    }

    return true;
}

Depth Search::nullMoveReduction(
    const SearchStack* ss,
    Depth depth,
    Score beta
) const {
    Depth reduction = NMP_BASE_REDUCTION;

    reduction += depth / 3 - 1;

    Score evalBonus = ss->staticEval - beta;

    if (evalBonus < 0) {
        evalBonus = 0;
    }

    if (evalBonus > NMP_MAX_EVAL_BONUS) {
        evalBonus = NMP_MAX_EVAL_BONUS;
    }

    reduction += evalBonus / 150;

    if (ss->improving) {
        reduction += 1;
    }
    else {
        reduction -= 1;
    }

    reduction = std::clamp<Depth>(reduction, 1, depth - 1);

    return reduction;
}

Depth Search::failHighReductionAdjustment(
    const SearchStack* ss,
    Depth depth,
    const Move& move,
    Score alpha,
    Score beta,
    Score score,
    Depth currentReduction
) const {
    if (currentReduction <= 0) {
        return 0;
    }

    if (depth < FHR_MIN_DEPTH) {
        return currentReduction;
    }

    if (!move.isQuiet()) {
        return currentReduction;
    }

    if (ss->inCheck) {
        return currentReduction;
    }

    const Score failHighMargin = score - alpha;

    if (failHighMargin >= 300) {
        return std::max<Depth>(1, currentReduction - 1);
    }

    if (failHighMargin >= 150 && ss->improving) {
        return std::max<Depth>(1, currentReduction - 1);
    }

    return 0;
}

Score Search::qsearch(
    Position& pos,
    Score alpha,
    Score beta,
    int qPly
) {
    stats.qnodes++;

    if (shouldStop()) {
        return lazyEvaluate(pos, pos.turn);
    }

    if (stopSearch) {
        return lazyEvaluate(pos, pos.turn);
    }

    const bool inCheckNow = inCheck(pos, pos.turn);

    Score standPat = lazyEvaluate(pos, pos.turn);

    if (qPly >= QS_MAX_DEPTH) {
        return standPat;
    }

    if (!inCheckNow) {
        if (standPat >= beta) {
            return beta;
        }

        if (standPat > alpha) {
            alpha = standPat;
        }
        
        const Score bigDelta = pieceValue[QUEEN] + QS_DELTA_MARGIN;

        if (standPat + bigDelta < alpha) {
            return alpha;
        }
    }

    MoveList moves;

    if (inCheckNow) {
        generateLegalMoves(pos, moves, pos.turn);
    }
    else {
        generateMoves(pos, moves, pos.turn);

        Move noTtMove;
        orderMoves(pos, moves, noTtMove);
    }

    for (int i = 0; i < moves.count; ++i) {
        if (stopSearch) {
            break;
        }

        const Move& move = moves[i];

        if (!inCheckNow && !move.isCapture() && !move.isPromotion()) {
            continue;
        }

        if (!inCheckNow) {
            if (move.isCapture() && !move.isPromotion()) {
                const Score captureDelta =
                    pieceValue[move.captured] + QS_DELTA_MARGIN;

                if (standPat + captureDelta < alpha) {
                    continue;
                }
            }

            if (move.isPromotion()) {
                const Score promotionDelta =
                    QS_PROMOTION_DELTA + QS_DELTA_MARGIN;

                if (standPat + promotionDelta < alpha) {
                    continue;
                }
            }

            if (move.isCapture() &&
                !move.isPromotion() &&
                move.captured != KING &&
                pieceValue[move.captured] + 200 < pieceValue[move.moved]) {
                continue;
            }
        }

        const int movingColor = pos.turn;

        History h = doMove(pos, move);

        if (!inCheckNow && inCheck(pos, movingColor)) {
            undoMove(pos, move, h);
            continue;
        }

        Score score = -qsearch(pos, -beta, -alpha, qPly + 1);

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
    SearchStack* ss,
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
        return qsearch(pos, alpha, beta, 0);
    }

    Move ttMove;
    Score ttScore = 0;
    Score ttEval = 0;
    bool ttPv = false;

    if (tt.probe(pos, depth, alpha, beta, ttScore, ttMove, ttEval, ttPv)) {
        stats.ttHits++;

        if (!isPv)
            return ttScore;
    }

    ss->ply = pos.ply;
    ss->moveCount = 0;
    ss->currentMove = Move();
    ss->ttPv = false;
    ss->inCheck = inCheck(pos, pos.turn);
    ss->staticEval = lazyEvaluate(pos, pos.turn);

    if (ss > stack) {
        ss->previousStaticEval = (ss - 1)->staticEval;
    }
    else {
        ss->previousStaticEval = ss->staticEval;
    }

    if (ss >= stack + 2) {
        ss->improving = ss->staticEval > (ss - 2)->staticEval;
    }
    else {
        ss->improving = false;
    }

    ss->ttPv = ttPv;

    if (canReverseFutilityPrune(ss, depth, beta, isPv)) {
        return ss->staticEval;
    }

    if (canNullMovePrune(ss, depth, beta, isPv)) {
        const Depth reduction = nullMoveReduction(ss, depth, beta);
        const Depth nullDepth = std::max<Depth>(0, depth - 1 - reduction);

        NullMoveHistory nh = doNullMove(pos);

        PVLine nullPv;

        Score nullScore = -negamax(
            pos,
            ss + 1,
            nullDepth,
            -beta,
            -beta + 1,
            false,
            nullPv
        );

        undoNullMove(pos, nh);

        if (stopSearch) {
            return ss->staticEval;
        }

        if (nullScore >= beta) {
            return beta;
        }
    }

    if (!ss->inCheck && !ttMove.isNone() && depth >= 4) {
        depth -= 1;
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

        ss->moveCount = i + 1;
        ss->currentMove = moves[i];

        PVLine childPv;

        History h = doMove(pos, move);

        const Depth newDepth = depth - 1;

        const Depth reduction = lmrReduction(
            ss,
            depth,
            ss->moveCount,
            isPv,
            move
        );

        Score score;

        const bool firstMove = i == 0;

        if (firstMove) {
            score = -negamax(
                pos,
                ss + 1,
                newDepth,
                -beta,
                -alpha,
                isPv,
                childPv
            );
        }
        else {
            if (reduction > 0) {
                const Depth reducedDepth = std::max<Depth>(0, newDepth - reduction);

                score = -negamax(
                    pos,
                    ss + 1,
                    reducedDepth,
                    -alpha - 1,
                    -alpha,
                    false,
                    childPv
                );

                if (!stopSearch && score > alpha) {
                    childPv.clear();

                    const Depth adjustedReduction = failHighReductionAdjustment(
                        ss,
                        depth,
                        move,
                        alpha,
                        beta,
                        score,
                        reduction
                    );

                    const Depth retryDepth = std::max<Depth>(0, newDepth - adjustedReduction);

                    score = -negamax(
                        pos,
                        ss + 1,
                        retryDepth,
                        -alpha - 1,
                        -alpha,
                        false,
                        childPv
                    );
                }
            }
            else {
                score = -negamax(
                    pos,
                    ss + 1,
                    newDepth,
                    -alpha - 1,
                    -alpha,
                    false,
                    childPv
                );
            }

            if (!stopSearch && score > alpha && score < beta) {
                childPv.clear();

                score = -negamax(
                    pos,
                    ss + 1,
                    newDepth,
                    -beta,
                    -alpha,
                    isPv,
                    childPv
                );
            }
        }

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

    if (stopSearch) {
        return bestScore;
    }

    TTBound bound = TT_EXACT;

    if (bestScore <= oldAlpha) {
        bound = TT_UPPER;
    }
    else if (bestScore >= beta) {
        bound = TT_LOWER;
    }

    tt.store(
        pos,
        depth,
        bestScore,
        ss->staticEval,
        bound,
        bestMove,
        isPv || ss->ttPv
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

        Score window = ASPIRATION_INITIAL_WINDOW;

        Score alpha = -INF;
        Score beta = INF;

        if (depth >= 4 && bestScore > -INF / 2 && bestScore < INF / 2) {
            alpha = bestScore - window;
            beta = bestScore + window;
        }

        bool depthCompleted = false;

        Move depthBestMove = moves[0];
        Score depthBestScore = -INF;
        PVLine rootPv;

        while (!stopSearch) {
            depthBestMove = moves[0];
            depthBestScore = -INF;
            rootPv.clear();

            for (int i = 0; i < MAX_PLY + 8; ++i) {
                stack[i].clear();
            }

            SearchStack* ss = stack;
            ss->clear();
            ss->ply = pos.ply;
            ss->staticEval = lazyEvaluate(pos, pos.turn);
            ss->inCheck = inCheck(pos, pos.turn);

            Score searchAlpha = alpha;

            for (int i = 0; i < moves.count; ++i) {
                if (stopSearch) {
                    break;
                }

                const Move& move = moves[i];

                ss->moveCount = i + 1;
                ss->currentMove = move;

                PVLine childPv;

                History h = doMove(pos, move);

                Score score;

                if (i == 0) {
                    score = -negamax(
                        pos,
                        ss + 1,
                        depth - 1,
                        -beta,
                        -searchAlpha,
                        true,
                        childPv
                    );
                }
                else {
                    score = -negamax(
                        pos,
                        ss + 1,
                        depth - 1,
                        -searchAlpha - 1,
                        -searchAlpha,
                        false,
                        childPv
                    );

                    if (!stopSearch && score > searchAlpha && score < beta) {
                        childPv.clear();

                        score = -negamax(
                            pos,
                            ss + 1,
                            depth - 1,
                            -beta,
                            -searchAlpha,
                            true,
                            childPv
                        );
                    }
                }

                undoMove(pos, move, h);

                if (stopSearch) {
                    break;
                }

                if (score > depthBestScore) {
                    depthBestScore = score;
                    depthBestMove = move;
                }

                if (score > searchAlpha) {
                    searchAlpha = score;

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

            if (depthBestScore <= alpha) {
                window *= 2;

                if (window > ASPIRATION_MAX_WINDOW) {
                    alpha = -INF;
                    beta = INF;
                }
                else {
                    alpha = std::max<Score>(-INF, depthBestScore - window);
                    beta = depthBestScore + window;
                }

                continue;
            }

            if (depthBestScore >= beta) {
                window *= 2;

                if (window > ASPIRATION_MAX_WINDOW) {
                    alpha = -INF;
                    beta = INF;
                }
                else {
                    alpha = depthBestScore - window;
                    beta = std::min<Score>(INF, depthBestScore + window);
                }

                continue;
            }

            depthCompleted = true;
            break;
        }

        if (!depthCompleted || stopSearch) {
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
            std::cout << " score mate " << signedMateDistanceMoves(bestScore);
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