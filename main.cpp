// ---=== LIBRARY IMPORTS ===---
#include <iostream> //for in/out
#include <vector> //for storing values
#include <string> //for move input
#include <chrono> //for time limits
#include <random> //for random side selection
#include <limits> //for input validation
#include <cstdlib> //for atoi / exit
#include <cctype> //for toupper
#include <unistd.h> //for isatty (GUI live output detection)
#include <cstdio> //for fileno
#include "board.hpp"
#include "evaluate.hpp"
#include "search.hpp"
#include "lang.hpp"

// ---=== GLOBAL VARIABLES ===---
Board game = {
    {
        'r','n','b','q','k','b','n','r',
        'p','p','p','p','p','p','p','p',
        ' ',' ',' ',' ',' ',' ',' ',' ',
        ' ',' ',' ',' ',' ',' ',' ',' ',
        ' ',' ',' ',' ',' ',' ',' ',' ',
        ' ',' ',' ',' ',' ',' ',' ',' ',
        'P','P','P','P','P','P','P','P',
        'R','N','B','Q','K','B','N','R'
    },
    -1, true, true, true, true
};
bool isVictory = false;
bool input_ended = false; //set when the player's input stream closes
bool playerWhite;  //true for white, false for black
short time_limit = 0;
int max_search_depth = 4; // how many plies the bot looks ahead
short bot_mode = 0; //aggressigve, offensive, defensive, guarding
bool verbose_mode = false; //true to show the bot's thinking
long player_time_used_ms = 0; //human side clock
long bot_time_used_ms = 0; //bot side clock

// ---=== CLI FLAGS ===---
// mirrors the Decoder-Malfunction-Simulator's flag handling (--en/--english ...)
bool flag_bot_mode = false, flag_side = false, flag_time = false, flag_verbose = false;
bool flag_depth = false;
bool side_random = false; //set by --side R

void print_usage()
{
    std::cout << "///===--- PickleBot ---===///\n";
    std::cout << tl("Opcje:", "Options:") << "\n";
    std::cout << "  --en, --ang, --english    " << tl("interfejs angielski (domyślny)", "English UI (default)") << "\n";
    std::cout << "  --pl, --polski            " << tl("interfejs polski", "Polish UI") << "\n";
    std::cout << "  --verbose, -v             " << tl("tryb verbose (myślenie bota)", "verbose mode (bot thinking)") << "\n";
    std::cout << "  --noverbose, --silent      " << tl("bez trybu verbose", "no verbose mode") << "\n";
    std::cout << "  --mode N                  " << tl("tryb bota 1-4", "bot mode 1-4") << "\n";
    std::cout << "  --side W|B|R              " << tl("twoja strona: Białe, Czarne lub Losowo", "your side: White, Black or Random") << "\n";
    std::cout << "  --time N                  " << tl("limit czasu w sekundach na stronę (0 = brak)", "time limit in seconds per side (0 = none)") << "\n";
    std::cout << "  --depth N                  " << tl("jak głęboko bot myśli naprzód (1-8)", "how many moves the bot looks ahead (1-8)") << "\n";
    std::cout << "  --help, -h                " << tl("ta pomoc", "this help") << "\n";
}

void parse_flags(int argc, char** argv)
{
    for (int i = 1; i < argc; i++)
    {
        std::string a = argv[i];
        if (a == "--en" || a == "--ang" || a == "--english") set_english(true);
        else if (a == "--pl" || a == "--polski") set_english(false);
        else if (a == "--verbose" || a == "-v") { verbose_mode = true; flag_verbose = true; }
        else if (a == "--noverbose" || a == "--silent") { verbose_mode = false; flag_verbose = true; }
        else if (a == "--mode" && i + 1 < argc)
        {
            bot_mode = (short)atoi(argv[++i]);
            if (bot_mode < 1 || bot_mode > 4) bot_mode = 1;
            flag_bot_mode = true;
        }
        else if (a == "--side" && i + 1 < argc)
        {
            std::string s = argv[++i];
            if (!s.empty())
            {
                char c = toupper(s[0]);
                if (c == 'W') playerWhite = true;
                else if (c == 'B') playerWhite = false;
                else side_random = true;
            }
            flag_side = true;
        }
        else if ((a == "--time" || a == "-t") && i + 1 < argc)
        {
            time_limit = (short)atoi(argv[++i]);
            if (time_limit < 0) time_limit = 0;
            flag_time = true;
        }
        else if (a == "--depth" && i + 1 < argc)
        {
            max_search_depth = atoi(argv[++i]);
            if (max_search_depth < 1) max_search_depth = 1;
            if (max_search_depth > 8) max_search_depth = 8;
            flag_depth = true;
        }
        else if (a == "--help" || a == "-h") { print_usage(); exit(0); }
    }
}

// ---=== PRE-DEFINITIONS ===---
void get_side_random();
void player_move();

// ---=== INPUT HELPERS ===---
// reads an integer, recovering from garbage input; returns false on EOF
bool read_int(short& value)
{
    if (!(std::cin >> value))
    {
        if (std::cin.eof()) { input_ended = true; return false; }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return false;
    }
    return true;
}
// read a number in [lo, hi], retrying on bad input; EOF returns lo
short prompt_number(short lo, short hi)
{
    short v;
    while (true)
    {
        if (!read_int(v))
        {
            if (input_ended) return lo;
            std::cout << tl("!!! Nieprawidłowy wybór - wybierz ponownie !!!",
                            "!!! Invalid choice - please reselect !!!") << "\n";
            continue;
        }
        if (v >= lo && v <= hi) return v;
        std::cout << tl("!!! Nieprawidłowy wybór - wybierz ponownie !!!",
                        "!!! Invalid choice - please reselect !!!") << "\n";
    }
}
// parse an algebraic square like "e4" into a board index; false on bad input
bool parse_square(const std::string& s, short& sq)
{
    if (s.size() != 2) return false;
    char f = s[0], r = s[1];
    if (f < 'a' || f > 'h' || r < '1' || r > '8') return false;
    sq = (8 - (r - '0')) * 8 + (f - 'a');
    return true;
}

// ---=== USER INTERACTION ===---
void greet()
{
    std::cout << "///===--- PickleBot ---===/// \n";
    std::cout << "/=- By C0m3b4ck under APL 2.0 -=/ \n";
    std::cout << tl("Wpisz `language en` (lub `jezyk pl`) w trakcie gry, aby przełączyć język.",
                    "Type `language pl` (or `jezyk en`) during a game to switch the language.") << "\n";
}
void goodbye()
{
    std::cout << "/> " << tl("Do widzenia od PickleBota", "Goodbye from PickleBot") << " /> \n"; 
}
void get_settings()
{
    // get bot mode
    if (!flag_bot_mode)
    {
        std::cout << tl("Podaj numer trybu bota:", "Input bot mode number:") << "\n";
        std::cout << "[1] " << tl("Agresywny (materiał)", "Aggressive (material)") << "\n";
        std::cout << "[2] " << tl("Ofensywny (koncentracja na królu)", "Offensive (focus on king)") << "\n";
        std::cout << "[3] " << tl("Defensywny (materiał)", "Defensive (material)") << "\n";
        std::cout << "[4] " << tl("Ochronny (defensywny nacisk na króla)", "Guarding (defensive focus on king)") << "\n";
        std::cout << tl("Twój wybór:", "Your choice:") << " ";
        bot_mode = prompt_number(1, 4);
        if (input_ended) return;
    }
    // get starting side
    if (flag_side)
    {
        if (side_random) get_side_random();
    }
    else
    {
        std::cout << tl("Podaj stronę (twoją):", "Input player (your) side:") << "\n";
        std::cout << "[1] " << tl("Białe", "White") << "\n";
        std::cout << "[2] " << tl("Czarne", "Black") << "\n";
        std::cout << "[3] " << tl("Losowo", "Random") << "\n";
        std::cout << tl("Twój wybór:", "Your choice:") << " ";
        short choice = prompt_number(1, 3);
        if (input_ended) return;
        switch (choice)
        {
            case 1: playerWhite = true; break;
            case 2: playerWhite = false; break;
            case 3: get_side_random(); break;
        }
    }
    // time limit value
    if (!flag_time)
    {
        std::cout << tl("Podaj limit czasu w sekundach na stronę (0 = brak):",
                        "Input time limit in seconds per side (0 for none):") << " ";
        if (!read_int(time_limit)) time_limit = 0;
        if (time_limit < 0) time_limit = 0;
    }
    // search depth value
    if (!flag_depth)
    {
        std::cout << tl("Podaj głębokość szukania (ile ruchów bot przewiduje naprzód, 1-8):",
                        "Input search depth (how many moves the bot predicts ahead, 1-8):") << " ";
        short depth = prompt_number(1, 8);
        if (input_ended) return;
        max_search_depth = depth;
    }
    // verbose mode
    if (!flag_verbose)
    {
        std::cout << tl("Włączyć tryb verbose (pokazywanie myślenia bota)?",
                        "Enable verbose mode (show bot thinking)?") << "\n";
        std::cout << "[1] " << tl("Tak", "Yes") << "\n";
        std::cout << "[2] " << tl("Nie", "No") << "\n";
        std::cout << tl("Twój wybór:", "Your choice:") << " ";
        short verb = prompt_number(1, 2);
        verbose_mode = (verb == 1);
    }
}
void print_board()
{
    for (short i = 0; i < 8; i++)
    {
        for (short j = 0; j < 8; j++)
        {
            std::cout << game.sq[i * 8 + j] << " ";
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}
// machine-readable state dump for the GUI: "@BOARD@ <W|B> " + 64 squares, row-major
void emit_board(bool turn_white)
{
    std::cout << "@BOARD@ " << (turn_white ? 'W' : 'B') << " ";
    for (short i = 0; i < 64; i++) std::cout << game.sq[i];
    std::cout << "\n";
}

void player_move()
{
    auto start_time = std::chrono::steady_clock::now();
    while (true)
    {
        std::cout << tl("Podaj ruch (skąd, dokąd): ", "Input move (from, to): ");
        std::string from, to;
        if (!(std::cin >> from >> to))
        {
            input_ended = true;
            return;
        }

        // runtime language switch (like the DMS `language en/pl` command)
        if (from == "language" || from == "lang" || from == "jezyk")
        {
            if (to == "en" || to == "english" || to == "ang") { set_english(true); std::cout << "Language set to English.\n"; }
            else if (to == "pl" || to == "polski") { set_english(false); std::cout << "Język ustawiony na polski.\n"; }
            else std::cout << tl("Użycie: `language en` / `language pl` (lub `jezyk en/pl`)",
                                 "Usage: `language en` / `language pl` (or `jezyk en/pl`)") << "\n";
            continue;
        }

        short from_num, to_num;
        if (!parse_square(from, from_num) || !parse_square(to, to_num))
        {
            std::cout << tl("!!! Nieprawidłowy ruch - zła nazwa pola !!!",
                            "!!! Invalid move - bad square name !!!") << "\n";
            continue;
        }

        // check 1: piece ownership
        if (!is_player_piece(game.sq[from_num], playerWhite))
        {
            std::cout << tl("!!! Nieprawidłowy ruch - to nie twoja figura !!!",
                            "!!! Invalid move - not your piece !!!") << "\n";
            continue;
        }
        // check 2: legal movement / path not obstructed
        if (!legal_move(game, from_num, to_num, playerWhite))
        {
            std::cout << tl("!!! Nieprawidłowy ruch - figura nie może tam się ruszyć !!!",
                            "!!! Invalid move - piece cannot move there !!!") << "\n";
            continue;
        }
        // check 3: king not left in check
        Board out;
        if (!try_move(game, from_num, to_num, playerWhite, out))
        {
            std::cout << tl("!!! Nieprawidłowy ruch - król byłby pod szachem !!!",
                            "!!! Invalid move - king would be in check !!!") << "\n";
            continue;
        }
        game = out;

        // track the player's clock
        if (time_limit > 0)
        {
            player_time_used_ms += std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time).count();
            long remaining = (long)time_limit * 1000 - player_time_used_ms;
            if (remaining < 0) remaining = 0;
            std::cout << tl("Czas gracza pozostał: ", "Player time left: ")
                      << remaining / 1000.0 << "s\n";
        }
        return;
    }
}

// ---=== GAME LOGIC ===---
void get_side_random()
{
    std::cout << tl("Losowanie strony... ", "Getting random side... ") << "\n";
    static std::mt19937 rng{std::random_device{}()};
    playerWhite = (rng() % 2 == 0);
    std::cout << tl("Zagrasz jako ", "You will play as ")
              << tl((playerWhite ? "Białe." : "Czarne."), (playerWhite ? "White." : "Black.")) << "\n";
}

// ends the game if the side about to move has no legal moves (checkmate or stalemate)
bool check_if_mate(bool side)
{
    if (!get_legal_moves(side).empty()) return false;
    if (king_in_check(game, side))
    {
        std::cout << (side ? tl("Czarne wygrywają przez mata!", "Black wins by checkmate!")
                           : tl("Białe wygrywają przez mata!", "White wins by checkmate!")) << "\n";
    }
    else
    {
        std::cout << tl("Pat - remis.", "Stalemate - it's a draw.") << "\n";
    }
    isVictory = true;
    return true;
}

void main_game_loop()
{
    bool bot_turn = !playerWhite; // the bot plays the opposite color
    while (!isVictory && !input_ended)
    {
        bool side = bot_turn ? !playerWhite : playerWhite;
        if (check_if_mate(side)) break;
        emit_board(side); //lets the GUI redraw the board
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
    if (isVictory)
    {
        emit_board(playerWhite);
        print_board();
    }
}

// ---=== PROGRAM ENTRY ===---
int main(int argc, char** argv)
{
    parse_flags(argc, argv);
    // when stdout is piped (e.g. to the GUI), flush every write so it arrives live
    if (!isatty(fileno(stdout))) std::cout.setf(std::ios::unitbuf);
    greet();
    get_settings();
    // let the GUI know the player's colour (needed for click-to-move)
    std::cout << "@SIDE@ " << (playerWhite ? 'W' : 'B') << "\n";
    main_game_loop();
    goodbye();
    return 0;
}
