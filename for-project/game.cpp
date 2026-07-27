#include <iostream>
using namespace std;

//game.cpp
//board has 64 squares
//a1 is position 1

// Position arrays for pieces on the board (0-indexed)
short white_pawn_pos[8] = {9, 10, 11, 12, 13, 14, 15, 16};
short white_rook_pos[2] = {1, 8};
short white_knight_pos[2] = {2, 7};
short white_bishop_pos[2] = {3, 6};
short white_queen_pos = 4;
short white_king_pos = 5;

short black_pawn_pos[8] = {49, 50, 51, 52, 53, 54, 55, 56};
short black_rook_pos[2] = {57, 64}; // Fixed typo from 'root' to 'rook'
short black_knight_pos[2] = {58, 63};
short black_bishop_pos[2] = {59, 62};
short black_queen_pos = 60;
short black_king_pos = 61;

short move_num = 0;
short userScore, botScore;
bool isUserWhite;
bool isUsersTurn;
bool isRunning = false;

char figure_chars[64] = {
    'R', 'K', 'B', 'Q', 'Y', 'B', 'K', 'R',
    'P', 'P', 'P', 'P', 'P', 'P', 'P', 'P',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    'p', 'p', 'p', 'p', 'p', 'p', 'p', 'p',
    'r', 'k', 'b', 'q', 'y', 'b', 'k', 'r'
};

// ---===FUNCTIONS===---
// --- DATA FUNCTIONS ---
void initialize_game_state() {

    move_num = 0;
    userScore = 0;
    botScore = 0;
    isUsersTurn = false;
    isRunning = false;

    char start[64] = {
        'R', 'K', 'B', 'Q', 'Y', 'B', 'K', 'R',
        'P', 'P', 'P', 'P', 'P', 'P', 'P', 'P',
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        'p', 'p', 'p', 'p', 'p', 'p', 'p', 'p',
        'r', 'k', 'b', 'q', 'y', 'b', 'k', 'r'
    };

    for (int i = 0; i < 64; i++) {
        figure_chars[i] = start[i];
    }

}
void update_turn() {
    isUsersTurn = !isUsersTurn;
}
void update_figures(short from, short to) {
    //checks which figure variable to change
    char fromPiece = figure_chars[from];
    // checks what to do if the piece is capital/lowercase
    // check if the move is even legal
    // calculate score earned for capturing
    if (isUsersTurn) {
        if (isUserWhite) {

        }
    }
    // actual swap
    figure_chars[from] = ' '; //overwrite from with blank space
    figure_chars[to] = fromPiece; //actually move the piece

}
void update_move(short from, short to) {
    update_figures(from,to);
    update_turn();
}
// --- PRINTING FUNCTIONS ---
void print_board() {
    move_num++;
    cout << "\n---=== ROUND " << move_num << " ===---\n";
    cout << "User score: " << userScore << "\n";
    cout << "Bot score: " << botScore << "\n\n";

    cout << "  A B C D E F G H\n";

    for (int i = 0; i < 64; i++) {
        if (i % 8 == 0) {
            cout << (8 - (i / 8)) << " ";
        }

        cout << figure_chars[i] << " ";

        if (i % 8 == 7) {
            cout << "\n";
        }
    }
}
void print_welcome() {
    cout<<"---=== Project codename 'PickleBot' ===--- \n";
    cout<<"A non-ML chess bot with a custom algorithm. \n";
    cout<<"/// By C0m3b4ck under APL 2.0 /// \n";
}
// --- USER CHOICE ---
void get_side_choice() {
    char tmp;
    cout<<"\n B/b for black, W/w for white";
    cout<<"\n Select a side, user: ";
    cin>>tmp;
    if (tmp == 'W' or tmp == 'w') {
        cout<<"\n Selected: white \n";
        isUserWhite = true;
        isRunning = true;
    }
    else if (tmp == 'B' or tmp == 'b') {
        cout<<"\n Selected: black \n";
        isUserWhite = false;
        isRunning = true;
    }
    else {
        cout<<"\n !!! Incorrect input !!! Please try again. \n";
        get_side_choice();
    }
}
void get_move_choice() {
    short from, to;
    char from_char, to_char;
    cout<<"\n --- YOUR MOVE --- \n";
    cout<<"Select piece to move (letter): ";
    cin>>from_char;
    cout<<"Select piece to move (number): ";
    cin>>from;
    cout<<"Select square to move to (letter): ";
    cin>>to_char;
    cout<<"Select square to move to (number): ";
    cin>>to;
    //debug
    cout << "\n" << "From: " << from_char << from << " To: " << to_char << to << "\n";
    //get array number
    if (from_char == 'b' || from_char == 'B') {
        from = from + 8;
    }
    else if (from_char == 'c' || from_char == 'C') {
        from = from + (8 + 2);
    }
    else if (from_char == 'd' || from_char == 'D') {
        from = from + (8 + 3);
    }
    else if (from_char == 'e' || from_char == 'E') {
        from = from + (8 + 4);
    }
    else if (from_char == 'f' || from_char == 'F') {
        from = from + (8 + 5);
    }
    else if (from_char == 'g' || from_char == 'G') {
        from = from + (8 + 6);
    }
    else if (from_char == 'h' || from_char == 'H') {
        from = from + (8 + 7);
    }
    if (from == to || from < 0 || from > 63 || to > 63 || to < 0) {
        cout<<"\n !!! Cannot move to same square !!! Please try again. \n";
        get_move_choice();
    }

    update_move(from, to);
}
// --- BOT FUNCTION ---
void bot_move() {
    // === CALLS FUNCTIONS LIKE bot_calculate_rook()
    //checks if the position a piece is currently in is in danger, if yes - MOVE
    //checks for a possible check move that will not get captured
    //calculates potential material gain
}

int main() {
    initialize_game_state();
    print_welcome();
    get_side_choice(); //triggers isRunning
    if (isUserWhite) {isUsersTurn = true;}
    else {isUsersTurn = false;}
    while (isRunning) {
        print_board();

        if (isUsersTurn) {
            get_move_choice();
        }
        else {
            bot_move();
            update_turn();
        }
    }
    return 0;
}
