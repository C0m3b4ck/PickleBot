// ---=== SEARCH ===---
#include <iostream>
#include <cctype>
#include "search.hpp"

// negamax with alpha-beta pruning; sets `aborted` if the time deadline is hit
long negamax(const Board& st, bool toMove, int depth, long alpha, long beta,
             const std::chrono::steady_clock::time_point& deadline, bool& aborted)
{
    if (aborted) return 0;
    if (std::chrono::steady_clock::now() > deadline) { aborted = true; return 0; }

    auto moves = generate_legal_moves(st, toMove);
    if (moves.empty())
        return king_in_check(st, toMove) ? -MATE : 0;
    if (depth == 0)
        return evaluate(st, toMove);

    // order moves by estimated value (captures first) for better pruning
    std::vector<std::pair<long, size_t>> ord;
    ord.reserve(moves.size());
    for (size_t i = 0; i < moves.size(); i++)
    {
        long key = 0;
        char target = st.sq[moves[i].second];
        if (target != ' ')
            key += piece_value(target) * PIECE_SCALE;
        else if (moves[i].second == st.ep && toupper(st.sq[moves[i].first]) == 'P'
                 && moves[i].first % 8 != moves[i].second % 8)
            key += PIECE_SCALE; // en passant capture
        if (toupper(st.sq[moves[i].first]) == 'P'
            && moves[i].second / 8 == (toMove ? 0 : 7))
            key += 5; // promoting pawn
        ord.push_back({key, i});
    }
    std::sort(ord.begin(), ord.end(),
              [](const std::pair<long, size_t>& a, const std::pair<long, size_t>& b)
              { return a.first > b.first; });

    long best = -INF;
    for (auto& e : ord)
    {
        Board child = st;
        make_move(child, moves[e.second].first, moves[e.second].second, toMove);
        long s = -negamax(child, !toMove, depth - 1, -beta, -alpha, deadline, aborted);
        if (aborted) return 0;
        if (s > best) best = s;
        if (best > alpha) alpha = best;
        if (alpha >= beta) break;
    }
    return best;
}

// searches the current global position to `depth`; `root_scores` optionally orders the root moves
SearchResult search_root(bool sideToMove, int depth,
                         const std::chrono::steady_clock::time_point& deadline,
                         const std::vector<long>* root_scores)
{
    auto moves = get_legal_moves(sideToMove);
    if (moves.empty()) return { -1, -1, false, depth };

    std::vector<size_t> idx(moves.size());
    for (size_t i = 0; i < moves.size(); i++) idx[i] = i;
    if (root_scores)
        std::sort(idx.begin(), idx.end(), [&](size_t a, size_t b)
                  { return (*root_scores)[a] > (*root_scores)[b]; });

    bool aborted = false;
    long best_score = -INF;
    short bf = moves[idx[0]].first, bt = moves[idx[0]].second;
    for (size_t k = 0; k < idx.size(); k++)
    {
        auto mv = moves[idx[k]];
        Board child = game;
        make_move(child, mv.first, mv.second, sideToMove);
        long s = -negamax(child, !sideToMove, depth - 1, -INF, INF, deadline, aborted);
        if (aborted) return { -1, -1, true, depth };
        if (s > best_score)
        {
            best_score = s;
            bf = mv.first;
            bt = mv.second;
        }
    }
    return { bf, bt, false, depth };
}

// after a move is applied to the board, predict the opponent's best reply
std::pair<short, short> predict_opponent_reply(bool botWhite)
{
    bool oppWhite = !botWhite;
    if (get_legal_moves(oppWhite).empty()) return {-1, -1};
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    SearchResult r = search_root(oppWhite, PREDICT_DEPTH, deadline, nullptr);
    return {r.from, r.to};
}

void bot_move()
{
    bool botWhite = !playerWhite;
    std::vector<std::pair<short, short>> moves = get_legal_moves(botWhite);
    if (moves.empty())
    {
        std::cout << "!!! Bot has no legal moves !!! \n";
        return;
    }

    size_t nmoves = moves.size();
    std::vector<long> scores(nmoves);
    std::vector<MoveEval> evals(nmoves);
    for (size_t i = 0; i < nmoves; i++)
        scores[i] = score_move(game, moves[i].first, moves[i].second, botWhite, &evals[i]);

    if (verbose_mode)
    {
        std::cout << "--- Bot thinking (mode: " << bot_mode_name() << ") ---\n";
        std::cout << "Legal moves found: " << nmoves << "\n";
        for (size_t i = 0; i < nmoves; i++)
        {
            const MoveEval& e = evals[i];
            std::cout << "  " << square_name(e.from) << "->" << square_name(e.to)
                      << " " << e.piece << (e.captured_piece != ' ' ? "x" : "-") << e.captured_piece
                      << " | capture=" << e.capture_score
                      << " proximity=" << e.proximity_score
                      << " expose=" << e.expose_penalty
                      << " check=" << e.check_bonus
                      << " => score " << e.final << "\n";
        }
    }

    short best_from = moves[0].first, best_to = moves[0].second;
    size_t best_index = 0;
    long best_score = scores[0];
    for (size_t i = 1; i < nmoves; i++)
    {
        if (scores[i] > best_score)
        {
            best_score = scores[i];
            best_from = moves[i].first;
            best_to = moves[i].second;
            best_index = i;
        }
    }

    auto start_time = std::chrono::steady_clock::now();

    if (verbose_mode)
    {
        const MoveEval& e = evals[best_index];
        std::cout << "  -> Depth 1 best: " << square_name(e.from) << "->" << square_name(e.to)
                  << " (score " << e.final << ")"
                  << " | capture=" << e.capture_score
                  << " proximity=" << e.proximity_score
                  << " expose=" << e.expose_penalty
                  << " check=" << e.check_bonus << "\n";
    }

    // time budget for this turn: a fraction of the remaining per-side clock
    long budget_ms = -1;
    if (time_limit > 0)
    {
        long remaining = (long)time_limit * 1000 - bot_time_used_ms;
        if (remaining <= 0) remaining = 0;
        budget_ms = remaining / 20; // a game lasts ~20 bot moves
        if (budget_ms < 50) budget_ms = 50;
        if (budget_ms > remaining) budget_ms = remaining; // never exceed the clock
    }
    auto deadline = budget_ms >= 0
        ? std::chrono::steady_clock::now() + std::chrono::milliseconds(budget_ms)
        : std::chrono::steady_clock::now() + std::chrono::hours(24);

    // iterative deepening: keep the last depth that finished in time
    int reached_depth = 1;
    SearchResult choice{ best_from, best_to, false, 1 };
    int max_depth = (time_limit > 0) ? MAX_SEARCH_DEPTH : DEFAULT_MAX_DEPTH;
    for (int depth = 2; depth <= max_depth; depth++)
    {
        if (std::chrono::steady_clock::now() >= deadline) break;
        SearchResult r = search_root(botWhite, depth, deadline, &scores);
        if (r.timed_out) break; // don't trust an incomplete search
        choice = r;
        reached_depth = depth;
    }
    best_from = choice.from;
    best_to = choice.to;

    long elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time).count();
    if (time_limit > 0) bot_time_used_ms += elapsed_ms;

    if (verbose_mode)
    {
        std::cout << "  -> Searched to depth " << reached_depth
                  << " in " << elapsed_ms << " ms"
                  << " (max " << max_depth << ")\n";
        std::cout << "  -> Final choice: " << square_name(best_from) << "->"
                  << square_name(best_to) << "\n";
        std::pair<short, short> reply = predict_opponent_reply(botWhite);
        if (reply.first >= 0)
        {
            std::cout << "  -> Predicted reply: " << square_name(reply.first)
                      << "->" << square_name(reply.second) << "\n";
        }
        else
        {
            std::cout << "  -> Predicted reply: none (opponent has no legal moves)\n";
        }
    }

    // apply the chosen move
    make_move(game, best_from, best_to, botWhite);

    if (time_limit > 0)
    {
        long remaining = (long)time_limit * 1000 - bot_time_used_ms;
        if (remaining < 0) remaining = 0;
        std::cout << "Bot time left: " << remaining / 1000.0 << "s\n";
    }

    std::cout << "Bot moves " << (char)('a' + best_from % 8) << (8 - best_from / 8)
              << " to " << (char)('a' + best_to % 8) << (8 - best_to / 8) << "\n";
}
