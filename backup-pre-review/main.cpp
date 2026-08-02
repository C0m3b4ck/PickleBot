// ---=== LIBRARY IMPORTS ===---
#include <iostream> //for in/out
#include <vector> //for storing values
#include <utility> //for std::pair
#include <thread> //for parallel move calculation
#include <functional> //for std::function
#include <mutex> //for the eval pool
#include <condition_variable> //for the eval pool
#include <atomic> //for the eval pool
#include <cctype> //for toupper
#include <chrono> //for time limits
#include <algorithm> //for move ordering
#include <random> //for random side selection

// ---=== GLOBAL VARIABLES ===---
bool isVictory = false;
bool input_ended = false; //set when the player's input stream closes
bool playerWhite;  //true for white, false for black
short time_limit = 0;
short bot_mode = 0; //aggressigve, offensive, defensive, guarding
bool verbose_mode = false; //true to show the bot's thinking
short en_passant_target = -1; //square a pawn can be captured en passant on, -1 if none
bool white_castle_ks = true; //white kingside castling available
bool white_castle_qs = true; //white queenside castling available
bool black_castle_ks = true; //black kingside castling available
bool black_castle_qs = true; //black queenside castling available
long player_time_used_ms = 0; //human side clock
long bot_time_used_ms = 0; //bot side clock
std::vector<char> board_characters = {
    'r','n','b','q','k','b','n','r',
    'p','p','p','p','p','p','p','p',
    ' ',' ',' ',' ',' ',' ',' ',' ',
    ' ',' ',' ',' ',' ',' ',' ',' ',
    ' ',' ',' ',' ',' ',' ',' ',' ',
    ' ',' ',' ',' ',' ',' ',' ',' ',
    'P','P','P','P','P','P','P','P',
    'R','N','B','Q','K','B','N','R'
};

// ---=== PRE-DEFINITIONS ===---
void get_side_random();
void player_move();
bool square_attacked(const std::vector<char>& b, short sq, bool by_white);

// ---=== USER INTERACTION ===---
void greet()
{
    std::cout << "///===--- PickleBot ---===/// \n";
    std::cout << "/=- By C0m3b4ck under APL 2.0 -=/ \n";
}
void goodbye()
{
    std::cout << "/> Goodbye from PickleBot /> \n"; 
}
void get_settings()
{
    // get bot mode
    std::cout << "Input bot mode number: \n";
    std::cout << "[1] Aggressive (material) \n";
    std::cout << "[2] Offensive (focus on king) \n";
    std::cout << "[3] Defensive (material) \n";
    std::cout << "[4] Guarding (defensive focus on king) \n";
    std::cout << "Your choice: ";
    while (bot_mode != 1 && bot_mode != 2 && bot_mode != 3 && bot_mode != 4)
    {
        std::cin >> bot_mode;
        if (bot_mode != 1 && bot_mode != 2 && bot_mode != 3 && bot_mode != 4)
        {
            std::cout << "!!! Invalid choice - please reselect !!! \n";
        }
    }
    // get starting side
    std::cout << "Input player (your) side: \n";
    std::cout << "[1] White \n";
    std::cout << "[2] Black \n";
    std::cout << "[3] Random \n";
    std::cout << "Your choice: ";
    short choice = 0;
    while (choice != 1 && choice != 2 && choice != 3)
    {
        std::cin >> choice;
        if (choice != 1 && choice != 2 && choice != 3)
        {
            std::cout << "!!! Invalid choice - please reselect !!! \n";
        }
        else
        {
            switch(choice)
            {
                case(1):
                    playerWhite = true;
                    break;
                case(2):
                    playerWhite = false;
                    break;
                case(3):
                    get_side_random();
            }
        }
    }
    // time limit value
    std::cout << "Input time limit in seconds per side (0 for none): ";
    std::cin >> time_limit;
    // verbose mode
    std::cout << "Enable verbose mode (show bot thinking)? \n";
    std::cout << "[1] Yes \n";
    std::cout << "[2] No \n";
    std::cout << "Your choice: ";
    short verb = 0;
    while (verb != 1 && verb != 2)
    {
        std::cin >> verb;
        if (verb != 1 && verb != 2)
        {
            std::cout << "!!! Invalid choice - please reselect !!! \n";
        }
    }
    verbose_mode = (verb == 1);
}
void print_board()
{
    for (short i = 0; i < 8; i++)
    {
        for (short j = 0; j < 8; j++)
        {
            std::cout << board_characters[i * 8 + j] << " ";
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}
bool is_white_piece(char c)
{
    return c >= 'A' && c <= 'Z';
}
bool is_player_piece(char c, bool white)
{
    if (c == ' ') return false;
    return is_white_piece(c) == white;
}
bool is_enemy_piece(char c, bool white)
{
    if (c == ' ') return false;
    return is_white_piece(c) != white;
}

bool legal_move(const std::vector<char>& b, short from, short to, bool white, short ep,
                bool wks, bool wqs, bool bks, bool bqs)
{
    char piece = b[from];
    if (piece == ' ') return false;
    if (is_player_piece(b[to], white)) return false;

    short fr = from / 8, fc = from % 8;
    short tr = to / 8, tc = to % 8;
    short dr = tr - fr, dc = tc - fc;
    short sdr = (dr > 0) - (dr < 0);
    short sdc = (dc > 0) - (dc < 0);
    short adr = dr > 0 ? dr : -dr;
    short adc = dc > 0 ? dc : -dc;

    switch (toupper(piece))
    {
        case 'P': // pawn
        {
            short step = white ? -1 : 1;
            short startrow = white ? 6 : 1;
            if (dc == 0) // forward
            {
                if (dr == step && b[to] == ' ') return true;
                if (fr == startrow && dr == 2 * step && b[to] == ' '
                    && b[from + step * 8] == ' ') return true;
                return false;
            }
            else // capture
            {
                if (adr == 1 && adc == 1 && dr == step)
                {
                    if (is_enemy_piece(b[to], white)) return true;
                    // en passant capture onto the target square
                    if (to == ep && is_enemy_piece(b[fr * 8 + tc], white)) return true;
                }
                return false;
            }
        }
        case 'R':
            if (fr != tr && fc != tc) return false;
            break;
        case 'B':
            if (adr != adc) return false;
            break;
        case 'Q':
            if (fr != tr && fc != tc && adr != adc) return false;
            break;
        case 'K':
            if (adr <= 1 && adc <= 1 && (adr + adc > 0)) return true;
            // castling: king slides two squares towards its own rook
            // white king on e1 (60), black king on e8 (4)
            if (white)
            {
                if (from == 60 && to == 62 && wks) // e1-g1
                {
                    if (b[61] != ' ' || b[62] != ' ') return false;
                    if (square_attacked(b, 60, false) || square_attacked(b, 61, false)
                        || square_attacked(b, 62, false)) return false;
                    return true;
                }
                if (from == 60 && to == 58 && wqs) // e1-c1
                {
                    if (b[57] != ' ' || b[58] != ' ' || b[59] != ' ') return false;
                    if (square_attacked(b, 60, false) || square_attacked(b, 59, false)
                        || square_attacked(b, 58, false)) return false;
                    return true;
                }
            }
            else
            {
                if (from == 4 && to == 6 && bks) // e8-g8
                {
                    if (b[5] != ' ' || b[6] != ' ') return false;
                    if (square_attacked(b, 4, true) || square_attacked(b, 5, true)
                        || square_attacked(b, 6, true)) return false;
                    return true;
                }
                if (from == 4 && to == 2 && bqs) // e8-c8
                {
                    if (b[1] != ' ' || b[2] != ' ' || b[3] != ' ') return false;
                    if (square_attacked(b, 4, true) || square_attacked(b, 3, true)
                        || square_attacked(b, 2, true)) return false;
                    return true;
                }
            }
            return false;
        case 'N':
            return (adr == 2 && adc == 1) || (adr == 1 && adc == 2);
        default:
            return false;
    }

    // sliding piece - check path is not obstructed
    short r = fr + sdr, c = fc + sdc;
    while (r != tr || c != tc)
    {
        if (b[r * 8 + c] != ' ') return false;
        r += sdr;
        c += sdc;
    }
    return true;
}

// convenience wrapper using the current global game state
bool legal_move(const std::vector<char>& b, short from, short to, bool white)
{
    return legal_move(b, from, to, white, en_passant_target,
                      white_castle_ks, white_castle_qs, black_castle_ks, black_castle_qs);
}

// a position as the search understands it: board + en passant + castling rights
struct Board
{
    std::vector<char> sq;
    short ep = -1;
    bool wks = true, wqs = true, bks = true, bqs = true;
};

// applies a move to a board and updates en passant target and castling rights
void make_move(std::vector<char>& b, short from, short to, bool white, short& ep,
               bool& wks, bool& wqs, bool& bks, bool& bqs)
{
    char moving = b[from];
    bool ep_capture = (to == ep) && toupper(moving) == 'P' && from % 8 != to % 8;
    short cap_sq = ep_capture ? from / 8 * 8 + to % 8 : to;
    char captured = b[cap_sq];

    // castling: the rook jumps across the king (e1/e8 to g/c file)
    if (toupper(moving) == 'K' && (to - from == 2 || to - from == -2))
    {
        short row = from / 8;
        if (to - from == 2) // kingside: rook from h-file to f-file
        {
            b[from + 1] = b[row * 8 + 7];
            b[row * 8 + 7] = ' ';
        }
        else // queenside: rook from a-file to d-file
        {
            b[from - 1] = b[row * 8];
            b[row * 8] = ' ';
        }
    }

    b[to] = moving;
    b[from] = ' ';
    if (ep_capture) b[cap_sq] = ' ';

    // update castling rights
    if (toupper(moving) == 'K')
    {
        if (white) { wks = false; wqs = false; }
        else { bks = false; bqs = false; }
    }
    else if (moving == 'R' || moving == 'r')
    {
        if (from == 63) wks = false;
        if (from == 56) wqs = false;
        if (from == 7) bks = false;
        if (from == 0) bqs = false;
    }
    if (ep_capture)
    {
        if (cap_sq == 63) wks = false;
        if (cap_sq == 56) wqs = false;
        if (cap_sq == 7) bks = false;
        if (cap_sq == 0) bqs = false;
    }
    else if (captured == 'R')
    {
        if (to == 63) wks = false;
        if (to == 56) wqs = false;
    }
    else if (captured == 'r')
    {
        if (to == 7) bks = false;
        if (to == 0) bqs = false;
    }

    // update en passant target after a two-square pawn push
    ep = -1;
    if (toupper(moving) == 'P' && (to / 8 - from / 8 == 2 || to / 8 - from / 8 == -2))
        ep = (from / 8 + to / 8) / 2 * 8 + to % 8;
}

void make_move(Board& st, short from, short to, bool white)
{
    make_move(st.sq, from, to, white, st.ep, st.wks, st.wqs, st.bks, st.bqs);
}

// is square `sq` attacked by a piece of color `by_white`?
bool square_attacked(const std::vector<char>& b, short sq, bool by_white)
{
    short r = sq / 8, c = sq % 8;

    // pawns attack from one row forward of the attacker's perspective
    char pawn = by_white ? 'P' : 'p';
    short dir = by_white ? 1 : -1; // white pawn attacks squares one row up, so source is one row down
    short pr = r + dir;
    if (pr >= 0 && pr < 8)
    {
        if (c - 1 >= 0 && b[pr * 8 + (c - 1)] == pawn) return true;
        if (c + 1 < 8 && b[pr * 8 + (c + 1)] == pawn) return true;
    }

    // knights
    char knight = by_white ? 'N' : 'n';
    static const short knight_dr[8] = {-2,-2,-1,-1,1,1,2,2};
    static const short knight_dc[8] = {-1,1,-2,2,-2,2,-1,1};
    for (short i = 0; i < 8; i++)
    {
        short nr = r + knight_dr[i], nc = c + knight_dc[i];
        if (nr >= 0 && nr < 8 && nc >= 0 && nc < 8 && b[nr * 8 + nc] == knight) return true;
    }

    // adjacent king
    char king = by_white ? 'K' : 'k';
    for (short dr = -1; dr <= 1; dr++)
        for (short dc = -1; dc <= 1; dc++)
        {
            if (dr == 0 && dc == 0) continue;
            short nr = r + dr, nc = c + dc;
            if (nr >= 0 && nr < 8 && nc >= 0 && nc < 8 && b[nr * 8 + nc] == king) return true;
        }

    // sliding rays: 0=N 1=S 2=W 3=E 4=NW 5=NE 6=SW 7=SE
    static const short ray_dr[8] = {-1,1,0,0,-1,-1,1,1};
    static const short ray_dc[8] = {0,0,-1,1,-1,1,-1,1};
    for (short d = 0; d < 8; d++)
    {
        short nr = r + ray_dr[d], nc = c + ray_dc[d];
        while (nr >= 0 && nr < 8 && nc >= 0 && nc < 8)
        {
            char p = b[nr * 8 + nc];
            if (p != ' ')
            {
                if (is_white_piece(p) == by_white)
                {
                    char u = toupper(p);
                    if (d < 4) // orthogonal: rook or queen
                    {
                        if (u == 'R' || u == 'Q') return true;
                    }
                    else // diagonal: bishop or queen
                    {
                        if (u == 'B' || u == 'Q') return true;
                    }
                }
                break;
            }
            nr += ray_dr[d];
            nc += ray_dc[d];
        }
    }
    return false;
}

bool king_in_check(const std::vector<char>& b, bool white)
{
    char king = white ? 'K' : 'k';
    for (short i = 0; i < 64; i++)
    {
        if (b[i] == king) return square_attacked(b, i, !white);
    }
    return true; // no king found - treat as check
}

void player_move()
{
    auto start_time = std::chrono::steady_clock::now();
    while (true)
    {
        std::cout << "Input move (from, to): ";
        std::string from, to;
        if (!(std::cin >> from >> to))
        {
            input_ended = true;
            return;
        }

        short from_num = (8 - (from[1] - '0')) * 8 + (from[0] - 'a');
        short to_num = (8 - (to[1] - '0')) * 8 + (to[0] - 'a');

        // check 1: piece ownership
        if (!is_player_piece(board_characters[from_num], playerWhite))
        {
            std::cout << "!!! Invalid move - not your piece !!! \n";
            continue;
        }
        // check 2: legal movement / path not obstructed
        if (!legal_move(board_characters, from_num, to_num, playerWhite))
        {
            std::cout << "!!! Invalid move - piece cannot move there !!! \n";
            continue;
        }
        // detect en passant capture
        bool en_passant_capture = false;
        short cap_square = -1;
        char saved_target = board_characters[to_num];
        char saved_cap = ' ';
        if (to_num == en_passant_target && toupper(board_characters[from_num]) == 'P'
            && from_num % 8 != to_num % 8)
        {
            en_passant_capture = true;
            cap_square = from_num / 8 * 8 + to_num % 8;
            saved_cap = board_characters[cap_square];
        }

        // check 3: king not left in check (simulation)
        board_characters[to_num] = board_characters[from_num];
        board_characters[from_num] = ' ';
        if (en_passant_capture) board_characters[cap_square] = ' ';
        if (king_in_check(board_characters, playerWhite))
        {
            board_characters[from_num] = board_characters[to_num];
            board_characters[to_num] = saved_target;
            if (en_passant_capture) board_characters[cap_square] = saved_cap;
            std::cout << "!!! Invalid move - king would be in check !!! \n";
            continue;
        }
        // undo the simulation, then apply the move for real
        board_characters[from_num] = board_characters[to_num];
        board_characters[to_num] = saved_target;
        if (en_passant_capture) board_characters[cap_square] = saved_cap;
        make_move(board_characters, from_num, to_num, playerWhite, en_passant_target,
                  white_castle_ks, white_castle_qs, black_castle_ks, black_castle_qs);

        // track the player's clock
        if (time_limit > 0)
        {
            player_time_used_ms += std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time).count();
            long remaining = (long)time_limit * 1000 - player_time_used_ms;
            if (remaining < 0) remaining = 0;
            std::cout << "Player time left: " << remaining / 1000.0 << "s\n";
        }
        return;
    }
}

// ---=== BOT LOGIC ===---
short piece_value(char c)
{
    switch (toupper(c))
    {
        case 'P': return 1;
        case 'N':
        case 'B': return 3;
        case 'R': return 5;
        case 'Q': return 9;
        case 'K': return 100;
        default: return 0;
    }
}

// persistent worker pool: keeps threads alive across moves (no per-call spawn cost)
class EvalPool
{
    size_t n;
    std::vector<std::thread> threads;
    std::vector<std::vector<char>> boards; // one private board per worker
    bool stop = false;
    unsigned round = 0;
    size_t begin_, end_;
    std::function<void(size_t, std::vector<char>&)> task_;
    std::atomic<size_t> next_{0};
    std::atomic<size_t> remaining_{0};
    std::mutex mtx_;
    std::condition_variable cv_go_, cv_done_;

    void worker(size_t t)
    {
        boards[t] = board_characters;
        unsigned my_round = 0;
        while (true)
        {
            {
                std::unique_lock<std::mutex> lk(mtx_);
                cv_go_.wait(lk, [&] { return stop || my_round < round; });
                if (stop) return;
                my_round = round;
            }
            boards[t] = board_characters; // fresh copy of the global board
            // claim tasks in batches to avoid atomic-counter contention
            static const size_t BATCH = 64;
            for (;;)
            {
                size_t start = next_.fetch_add(BATCH);
                if (start >= end_) break;
                size_t stop = std::min(start + BATCH, end_);
                for (size_t i = start; i < stop; i++) task_(i, boards[t]);
            }
            if (remaining_.fetch_sub(1) == 1) cv_done_.notify_all();
        }
    }

public:
    explicit EvalPool(size_t nthreads) : n(nthreads), boards(nthreads)
    {
        for (size_t t = 0; t < n; t++) threads.emplace_back([this, t] { worker(t); });
    }
    ~EvalPool()
    {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            stop = true;
        }
        round++;
        cv_go_.notify_all();
        for (auto& t : threads) t.join();
    }
    // run task(i, workerBoard) for every i in [begin, end), blocks until finished
    void run(size_t begin, size_t end, std::function<void(size_t, std::vector<char>&)> task)
    {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            begin_ = begin;
            end_ = end;
            task_ = std::move(task);
            round++;
        }
        next_.store(begin);
        remaining_.store(n);
        cv_go_.notify_all();
        std::unique_lock<std::mutex> lk(mtx_);
        cv_done_.wait(lk, [&] { return remaining_ == 0; });
    }
};

EvalPool& eval_pool()
{
    unsigned hw = std::thread::hardware_concurrency();
    static EvalPool pool(hw ? hw : 1);
    return pool;
}

// only engage parallel scoring when the workload is large enough to beat thread latency
const size_t PARALLEL_SCORE_THRESHOLD = 512;

std::vector<std::pair<short, short>> generate_legal_moves(const std::vector<char>& b, bool white,
    short ep, bool wks, bool wqs, bool bks, bool bqs)
{
    std::vector<std::pair<short, short>> moves;
    for (short from = 0; from < 64; from++)
    {
        if (!is_player_piece(b[from], white)) continue;
        for (short to = 0; to < 64; to++)
        {
            if (from == to) continue;
            if (!legal_move(b, from, to, white, ep, wks, wqs, bks, bqs)) continue;
            // reject moves that leave own king in check
            std::vector<char> work = b;
            bool ep_capture = (to == ep) && toupper(work[from]) == 'P' && from % 8 != to % 8;
            short cap_sq = ep_capture ? from / 8 * 8 + to % 8 : to;
            bool castle = toupper(work[from]) == 'K' && (to - from == 2 || to - from == -2);
            short rook_from = 0, rook_to = 0;
            if (castle)
            {
                short row = from / 8;
                rook_from = (to - from == 2) ? row * 8 + 7 : row * 8;
                rook_to = (to - from == 2) ? from + 1 : from - 1;
                char r = work[rook_from];
                work[rook_from] = ' ';
                work[rook_to] = r;
            }
            work[to] = work[from];
            work[from] = ' ';
            if (ep_capture) work[cap_sq] = ' ';
            if (!king_in_check(work, white)) moves.push_back({from, to});
        }
    }
    return moves;
}

std::vector<std::pair<short, short>> get_legal_moves(bool white)
{
    return generate_legal_moves(board_characters, white, en_passant_target,
                                white_castle_ks, white_castle_qs, black_castle_ks, black_castle_qs);
}

// breakdown of one move's evaluation, used by verbose mode
struct MoveEval
{
    short from, to;
    char piece, captured_piece;
    long capture_score;
    long proximity_score;
    long expose_penalty;
    long check_bonus;
    long final;
};

// convert a board index (0 = a8) to algebraic square name like "e4"
std::string square_name(short sq)
{
    std::string s(2, ' ');
    s[0] = 'a' + sq % 8;
    s[1] = '0' + (8 - sq / 8);
    return s;
}

const char* bot_mode_name()
{
    switch (bot_mode)
    {
        case 1: return "Aggressive (material)";
        case 2: return "Offensive (focus on king)";
        case 3: return "Defensive (material)";
        case 4: return "Guarding (defensive focus on king)";
        default: return "Unknown";
    }
}

// scores one move on a thread-local board copy; safe to call from multiple threads
long score_move(std::vector<char>& b, short from, short to, bool botWhite, MoveEval* eval = nullptr)
{
    char moving = b[from];
    char save_to = b[to];

    bool en_passant_capture = false;
    short cap_square = -1;
    char save_cap = ' ';
    if (to == en_passant_target && toupper(moving) == 'P' && from % 8 != to % 8)
    {
        en_passant_capture = true;
        cap_square = from / 8 * 8 + to % 8;
        save_cap = b[cap_square];
    }

    // simulate the move
    b[to] = moving;
    b[from] = ' ';
    if (en_passant_capture) b[cap_square] = ' ';

    // 1. capture material
    long capture_score = piece_value(en_passant_capture ? save_cap : save_to) * 10;

    // 2. how many moves to attack the enemy king (piece proximity)
    long proximity_score = 0;
    char enemy_king = botWhite ? 'K' : 'k';
    short kingpos = -1;
    for (short i = 0; i < 64; i++)
    {
        if (b[i] == enemy_king)
        {
            kingpos = i;
            break;
        }
    }
    if (kingpos >= 0)
    {
        short dr = kingpos / 8 - to / 8;
        short dc = kingpos % 8 - to % 8;
        dr = dr > 0 ? dr : -dr;
        dc = dc > 0 ? dc : -dc;
        proximity_score = 14 - (dr + dc);
    }

    // 3. does the move expose the bot's king
    long expose_penalty = king_in_check(b, botWhite) ? 1 : 0;

    // 4. does the move check the enemy king
    long check_bonus = king_in_check(b, !botWhite) ? 1 : 0;

    // undo the simulation
    b[from] = moving;
    b[to] = save_to;
    if (en_passant_capture) b[cap_square] = save_cap;

    long final = 0;
    switch (bot_mode)
    {
        case 1: // Aggressive - material
            final = capture_score * 3 + proximity_score + check_bonus * 10 - expose_penalty * 20;
            break;
        case 2: // Offensive - focus on king
            final = capture_score + proximity_score * 3 + check_bonus * 40 - expose_penalty * 20;
            break;
        case 3: // Defensive - material
            final = capture_score * 3 - expose_penalty * 50 + check_bonus * 5;
            break;
        case 4: // Guarding - defensive focus on king
            final = capture_score - expose_penalty * 70 + check_bonus * 10;
            break;
        default:
            final = capture_score + proximity_score + check_bonus * 10 - expose_penalty * 20;
            break;
    }
    if (eval)
    {
        eval->from = from;
        eval->to = to;
        eval->piece = moving;
        eval->captured_piece = en_passant_capture ? save_cap : save_to;
        eval->capture_score = capture_score;
        eval->proximity_score = proximity_score;
        eval->expose_penalty = expose_penalty;
        eval->check_bonus = check_bonus;
        eval->final = final;
    }
    return final;
}

// ---=== SEARCH ===---
const long INF = 1000000000;
const long MATE = 1000000;
const int MAX_SEARCH_DEPTH = 6; // deepest the bot will ever look
const int DEFAULT_MAX_DEPTH = 4; // depth used when no time limit is set
const int PREDICT_DEPTH = 3; // depth used for predicted opponent replies

// static evaluation of a position from `sideToMove`'s point of view
long evaluate(const std::vector<char>& b, bool sideToMove)
{
    long mat = 0;
    short enemy_king = -1;
    for (short i = 0; i < 64; i++)
    {
        char c = b[i];
        if (c == ' ') continue;
        bool mine = is_white_piece(c) == sideToMove;
        long v = piece_value(c) * 10;
        mat += mine ? v : -v;
        if (!mine && toupper(c) == 'K') enemy_king = i;
    }
    // how close our pieces are to the enemy king
    long proximity = 0;
    if (enemy_king >= 0)
    {
        for (short i = 0; i < 64; i++)
        {
            char c = b[i];
            if (c == ' ' || is_white_piece(c) != sideToMove) continue;
            short dr = enemy_king / 8 - i / 8;
            short dc = enemy_king % 8 - i % 8;
            dr = dr > 0 ? dr : -dr;
            dc = dc > 0 ? dc : -dc;
            proximity += 14 - (dr + dc);
        }
    }
    long my_check = king_in_check(b, sideToMove) ? 1 : 0;
    long enemy_check = king_in_check(b, !sideToMove) ? 1 : 0;
    switch (bot_mode)
    {
        case 1: return mat * 3 + proximity + enemy_check * 10 - my_check * 20;
        case 2: return mat + proximity * 3 + enemy_check * 40 - my_check * 20;
        case 3: return mat * 3 - my_check * 50 + enemy_check * 5;
        case 4: return mat - my_check * 70 + enemy_check * 10;
        default: return mat + proximity + enemy_check * 10 - my_check * 20;
    }
}

// negamax with alpha-beta pruning; sets `aborted` if the time deadline is hit
long negamax(const Board& st, bool toMove, int depth, long alpha, long beta,
             const std::chrono::steady_clock::time_point& deadline, bool& aborted)
{
    if (aborted) return 0;
    if (std::chrono::steady_clock::now() > deadline) { aborted = true; return 0; }

    auto moves = generate_legal_moves(st.sq, toMove, st.ep, st.wks, st.wqs, st.bks, st.bqs);
    if (moves.empty())
        return king_in_check(st.sq, toMove) ? -MATE : 0;
    if (depth == 0)
        return evaluate(st.sq, toMove);

    // order moves by estimated value (captures first) for better pruning
    std::vector<std::pair<long, size_t>> ord;
    ord.reserve(moves.size());
    for (size_t i = 0; i < moves.size(); i++)
    {
        long key = 0;
        char target = st.sq[moves[i].second];
        if (target != ' ')
            key += piece_value(target) * 10;
        else if (moves[i].second == st.ep && toupper(st.sq[moves[i].first]) == 'P'
                 && moves[i].first % 8 != moves[i].second % 8)
            key += 10; // en passant capture
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

struct SearchResult
{
    short from = -1, to = -1;
    bool timed_out = false;
    int depth = 0;
};

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
        Board child{board_characters, en_passant_target,
                    white_castle_ks, white_castle_qs, black_castle_ks, black_castle_qs};
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

    if (nmoves < PARALLEL_SCORE_THRESHOLD)
    {
        std::vector<char> b = board_characters;
        for (size_t i = 0; i < nmoves; i++)
            scores[i] = score_move(b, moves[i].first, moves[i].second, botWhite, &evals[i]);
    }
    else
    {
        eval_pool().run(0, nmoves, [&](size_t i, std::vector<char>& b) {
            scores[i] = score_move(b, moves[i].first, moves[i].second, botWhite, &evals[i]);
        });
    }

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
    make_move(board_characters, best_from, best_to, botWhite, en_passant_target,
              white_castle_ks, white_castle_qs, black_castle_ks, black_castle_qs);

    if (time_limit > 0)
    {
        long remaining = (long)time_limit * 1000 - bot_time_used_ms;
        if (remaining < 0) remaining = 0;
        std::cout << "Bot time left: " << remaining / 1000.0 << "s\n";
    }

    std::cout << "Bot moves " << (char)('a' + best_from % 8) << (8 - best_from / 8)
              << " to " << (char)('a' + best_to % 8) << (8 - best_to / 8) << "\n";
}

// ---=== GAME LOGIC ===---
void get_side_random()
{
    std::cout << "Getting random side... \n";
    static std::mt19937 rng{std::random_device{}()};
    playerWhite = (rng() % 2 == 0);
    std::cout << "You will play as " << (playerWhite ? "White" : "Black") << ".\n";
}

// ends the game if the side about to move has no legal moves (checkmate or stalemate)
bool check_if_mate(bool side)
{
    if (!get_legal_moves(side).empty()) return false;
    if (king_in_check(board_characters, side))
    {
        std::cout << (side ? "Black" : "White") << " wins by checkmate!\n";
    }
    else
    {
        std::cout << "Stalemate - it's a draw.\n";
    }
    isVictory = true;
    return true;
}

void main_game_loop() //note - consider return to be then returned by main() for better debug etc.
{
    bool bot_turn = !playerWhite; // the bot plays the opposite color
    while (!isVictory && !input_ended)
    {
        bool side = bot_turn ? !playerWhite : playerWhite;
        if (check_if_mate(side)) break;
        if (bot_turn)
        {
            bot_move();
        }
        else
        {
            print_board(); //outputs board
            player_move(); //gets user input, validates
            print_board(); //outputs board
        }
        bot_turn = !bot_turn;
    }
    if (isVictory) print_board();
}

// ---=== PROGRAM ENTRY ===---
int main()
{
    greet();
    get_settings();
    main_game_loop();
    goodbye();
    return 0;
}