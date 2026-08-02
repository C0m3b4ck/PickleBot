// ---=== SEARCH ===---
#include <iostream>
#include <cctype>
#include "search.hpp"
#include "lang.hpp"

TTEntry g_tt[TT_SIZE];

// deterministic 64-bit key for a position + side to move (FNV-1a over the board)
uint64_t position_key(const Board& b, bool toMove)
{
    uint64_t h = 1469598103934665603ULL;
    const uint64_t fnv = 1099511628211ULL;
    for (int i = 0; i < 64; i++)
    {
        h ^= (unsigned char)b.sq[i];
        h *= fnv;
    }
    h ^= (uint64_t)(b.ep + 1); h *= fnv;
    h ^= b.wks ? 1 : 0; h *= fnv;
    h ^= b.wqs ? 1 : 0; h *= fnv;
    h ^= b.bks ? 1 : 0; h *= fnv;
    h ^= b.bqs ? 1 : 0; h *= fnv;
    h ^= toMove ? 0x9E3779B97F4A7C15ULL : 0; h *= fnv;
    return h;
}

// captures/promotions-only search at the horizon, so tactics aren't cut off
long quiesce(const Board& st, bool toMove, long alpha, long beta,
             const std::chrono::steady_clock::time_point& deadline, bool& aborted)
{
    if (aborted) return 0;
    if (std::chrono::steady_clock::now() > deadline) { aborted = true; return 0; }

    // stand pat: if even this quiet position beats beta, the opponent won't
    // let us reach it
    long stand = evaluate(st, toMove);
    if (stand >= beta) return stand;
    if (stand > alpha) alpha = stand;

    auto moves = generate_legal_moves(st, toMove);
    // keep only tactical moves: captures, en passant, pawn promotions
    std::vector<std::pair<long, size_t>> ord;
    ord.reserve(moves.size());
    for (size_t i = 0; i < moves.size(); i++)
    {
        char target = st.sq[moves[i].second];
        bool is_capture = target != ' '
            || (moves[i].second == st.ep && toupper(st.sq[moves[i].first]) == 'P'
                && moves[i].first % 8 != moves[i].second % 8);
        bool is_promo = toupper(st.sq[moves[i].first]) == 'P'
            && moves[i].second / 8 == (toMove ? 0 : 7);
        if (!is_capture && !is_promo) continue;
        long key = piece_value(target) * PIECE_SCALE;
        if (key <= 0) key = 1; // promotions / en passant
        ord.push_back({key, i});
    }
    std::sort(ord.begin(), ord.end(),
              [](const std::pair<long, size_t>& a, const std::pair<long, size_t>& b)
              { return a.first > b.first; });

    for (auto& e : ord)
    {
        Board child = st;
        make_move(child, moves[e.second].first, moves[e.second].second, toMove);
        long s = -quiesce(child, !toMove, -beta, -alpha, deadline, aborted);
        if (aborted) return 0;
        if (s > alpha) { alpha = s; if (alpha >= beta) break; }
    }
    return alpha;
}

// negamax with alpha-beta pruning; sets `aborted` if the time deadline is hit
long negamax(const Board& st, bool toMove, int depth, long alpha, long beta,
             const std::chrono::steady_clock::time_point& deadline, bool& aborted)
{
    if (aborted) return 0;
    if (std::chrono::steady_clock::now() > deadline) { aborted = true; return 0; }

    long alpha_orig = alpha;

    // transposition table probe: reuse a finished search at >= this depth
    uint64_t key = position_key(st, toMove);
    TTEntry& ent = g_tt[key & TT_MASK];
    if (ent.key == key && ent.depth >= depth)
    {
        if (ent.flag == 0) return ent.score;
        if (ent.flag == 1 && ent.score > alpha) alpha = ent.score;
        if (ent.flag == 2 && ent.score < beta) beta = ent.score;
        if (alpha >= beta) return ent.score;
    }

    auto moves = generate_legal_moves(st, toMove);
    long result;
    unsigned char flag = 0;
    bool store = false;
    if (moves.empty())
    {
        result = king_in_check(st, toMove) ? -MATE : 0;
        flag = 0;
        store = true;
    }
    else if (depth == 0)
    {
        result = quiesce(st, toMove, alpha, beta, deadline, aborted);
        if (aborted) return 0;
        store = false; // quiesce returns a bound, not an exact score
    }
    else
    {
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
        if (best <= alpha_orig) flag = 2; // fail-low: upper bound
        else if (best >= beta) flag = 1;  // fail-high: lower bound
        else flag = 0;                    // exact
        result = best;
        store = true;
    }

    if (store && !aborted)
    {
        ent.key = key;
        ent.score = result;
        ent.depth = (short)depth;
        ent.flag = flag;
    }
    return result;
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
    SearchResult r = search_root(oppWhite, std::min(PREDICT_DEPTH, max_search_depth), deadline, nullptr);
    return {r.from, r.to};
}

void bot_move()
{
    bool botWhite = !playerWhite;
    std::vector<std::pair<short, short>> moves = get_legal_moves(botWhite);
    if (moves.empty())
    {
        std::cout << tl("!!! Bot nie ma legalnych ruchów !!!", "!!! Bot has no legal moves !!!") << "\n";
        return;
    }

    size_t nmoves = moves.size();
    std::vector<long> scores(nmoves);
    std::vector<MoveEval> evals(nmoves);
    for (size_t i = 0; i < nmoves; i++)
        scores[i] = score_move(game, moves[i].first, moves[i].second, botWhite, &evals[i]);

    if (verbose_mode)
    {
        std::cout << "--- " << tl("Myślenie bota (tryb: ", "Bot thinking (mode: ") << bot_mode_name() << ") ---\n";
        std::cout << tl("Znaleziono legalnych ruchów: ", "Legal moves found: ") << nmoves << "\n";
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
        std::cout << "  -> " << tl("Głębokość 1 najlepszy: ", "Depth 1 best: ")
                  << square_name(e.from) << "->" << square_name(e.to)
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
    int max_depth = max_search_depth;
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
        std::cout << "  -> " << tl("Przeszukano do głębokości ", "Searched to depth ")
                  << reached_depth << " " << tl("w ", "in ") << elapsed_ms << " ms"
                  << " (max " << max_depth << ")\n";
        std::cout << "  -> " << tl("Ostateczny wybór: ", "Final choice: ")
                  << square_name(best_from) << "->" << square_name(best_to) << "\n";
        std::pair<short, short> reply = predict_opponent_reply(botWhite);
        if (reply.first >= 0)
        {
            std::cout << "  -> " << tl("Przewidywana odpowiedź: ", "Predicted reply: ")
                      << square_name(reply.first) << "->" << square_name(reply.second) << "\n";
        }
        else
        {
            std::cout << "  -> " << tl("Przewidywana odpowiedź: brak (przeciwnik nie ma legalnych ruchów)",
                                       "Predicted reply: none (opponent has no legal moves)") << "\n";
        }
    }

    // apply the chosen move
    make_move(game, best_from, best_to, botWhite);

    if (time_limit > 0)
    {
        long remaining = (long)time_limit * 1000 - bot_time_used_ms;
        if (remaining < 0) remaining = 0;
        std::cout << tl("Czas bota pozostał: ", "Bot time left: ") << remaining / 1000.0 << "s\n";
    }

    std::cout << tl("Ruch bota: ", "Bot moves ")
              << (char)('a' + best_from % 8) << (8 - best_from / 8)
              << " " << tl("na ", "to ")
              << (char)('a' + best_to % 8) << (8 - best_to / 8) << "\n";
}
