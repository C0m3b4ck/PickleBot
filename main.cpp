// ---=== LIBRARY IMPORTS ===---
#include <iostream> //for in/out
#include <vector> //for storing values

// ---=== GLOBAL VARIABLES ===---
bool isVictory = false;
bool playerWhite;  //true for white, false for black
short time_limit = 0;
short bot_mode = 0; //aggressigve, offensive, defensive, guarding
std::vector<char> board_characters = {
    'r','k','b','q','x','b','k','r',
    'p','p','p','p','p','p','p','p',
    ' ',' ',' ',' ',' ',' ',' ',' ',
    ' ',' ',' ',' ',' ',' ',' ',' ',
    ' ',' ',' ',' ',' ',' ',' ',' ',
    ' ',' ',' ',' ',' ',' ',' ',' ',
    'P','P','P','P','P','P','P','P',
    'R','K','B','Q','X','B','K','R'
}; //NOTE: reverse kind and queen if !playerWhite

// ---=== PRE-DEFINITIONS ===---
void get_side_random();
void player_move();

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
    while (bot_mode != 1 && bot_mode != 2 && bot_mode != 3)
    {
        std::cin >> bot_mode;
        if (bot_mode != 1 && bot_mode != 2 && bot_mode != 3)
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
    short choice = ' ';
    while (choice != '1' && choice != '2' && choice != '3')
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
void player_move()
{
    //get user input
    //check if the move is legal
    //if yes, proceed
    std::cout << "Not done yet! \n";
}

// ---=== BOT LOGIC ===---
void find_best_move()
{
    std::cout << "Not done yet! \n";
}
void get_legal_moves(bool is_player)
{
    std::cout << "Not done yet! \n";
}
void bot_move()
{
    get_legal_moves(false);
    find_best_move();
}

// ---=== GAME LOGIC ===---
void get_side_random()
{
    std::cout << "Getting random side... \n";
}
void main_game_loop() //note - consider return to be then returned by main() for better debug etc.
{
    while (!isVictory)
    {
        print_board(); //outputs board
        player_move(); //gets user input, validates
        print_board(); //outputs board
        bot_move(); //runs bot logic
    }
}
void check_if_mate() //modifies isVictory into true if mate occurs
{
    // checking function here  
    std::cout << "Not done yet! \n";  
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