// ---=== LIBRARY IMPORTS ===---
#include <iostream> //for in/out
#include <vector> //for storing values
#include <string> //for move input
#include <chrono> //for time limits
#include <random> //for random side selection
#include <limits> //for input validation
#include "board.hpp"
#include "evaluate.hpp"
#include "search.hpp"

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
short bot_mode = 0; //aggressigve, offensive, defensive, guarding
bool verbose_mode = false; //true to show the bot's thinking
long player_time_used_ms = 0; //human side clock
long bot_time_used_ms = 0; //bot side clock

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
            std::cout << "!!! Invalid choice - please reselect !!! \n";
            continue;
        }
        if (v >= lo && v <= hi) return v;
        std::cout << "!!! Invalid choice - please reselect !!! \n";
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
    bot_mode = prompt_number(1, 4);
    if (input_ended) return;
    // get starting side
    std::cout << "Input player (your) side: \n";
    std::cout << "[1] White \n";
    std::cout << "[2] Black \n";
    std::cout << "[3] Random \n";
    std::cout << "Your choice: ";
    short choice = prompt_number(1, 3);
    if (input_ended) return;
    switch (choice)
    {
        case 1: playerWhite = true; break;
        case 2: playerWhite = false; break;
        case 3: get_side_random(); break;
    }
    // time limit value
    std::cout << "Input time limit in seconds per side (0 for none): ";
    if (!read_int(time_limit)) time_limit = 0;
    if (time_limit < 0) time_limit = 0;
    // verbose mode
    std::cout << "Enable verbose mode (show bot thinking)? \n";
    std::cout << "[1] Yes \n";
    std::cout << "[2] No \n";
    std::cout << "Your choice: ";
    short verb = prompt_number(1, 2);
    verbose_mode = (verb == 1);
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

        short from_num, to_num;
        if (!parse_square(from, from_num) || !parse_square(to, to_num))
        {
            std::cout << "!!! Invalid move - bad square name !!! \n";
            continue;
        }

        // check 1: piece ownership
        if (!is_player_piece(game.sq[from_num], playerWhite))
        {
            std::cout << "!!! Invalid move - not your piece !!! \n";
            continue;
        }
        // check 2: legal movement / path not obstructed
        if (!legal_move(game, from_num, to_num, playerWhite))
        {
            std::cout << "!!! Invalid move - piece cannot move there !!! \n";
            continue;
        }
        // check 3: king not left in check
        Board out;
        if (!try_move(game, from_num, to_num, playerWhite, out))
        {
            std::cout << "!!! Invalid move - king would be in check !!! \n";
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
            std::cout << "Player time left: " << remaining / 1000.0 << "s\n";
        }
        return;
    }
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
    if (king_in_check(game, side))
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

void main_game_loop()
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
