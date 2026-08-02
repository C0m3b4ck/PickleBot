#include <iostream>
#include <string>

using namespace std;

// Global Variables
char board[8][8];
bool king = false;
int rows, cols;
int moves = 0;

// Function to print the chessboard
void print_board() {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (board[i][j] == '\0')
                cout << "X ";
            else
                cout << board[i][j] << " ";
        }
        cout << endl;
    }
}

// Function to check a move and return whether it is legal or not
bool is_legal_move(int from, int to) {
    if (board[from][from] == '\0')
        return true; // Pawn promotion

    char piece = board[from][from];
    if (piece == 'R') { // Red King
        if ((abs(from - to) > 1 && abs(to - from) == 1) || (to - from) % 8 == 1)
            return false;
    }
    else if (piece == 'B') { // Black King
        if ((abs(from - to) > 1 && abs(to - from) == 1) || (to - from) % 8 == 7)
            return false;
    }

    char target_piece = board[to][to];
    if (target_piece != '\0' && piece == target_piece)
        return false; // If it is, then move cannot be legal
    else {
        if ((piece == 'R' || piece == 'r') && to - from == 1)
            return true; // Move up one square for Red pawns
        if ((piece == 'B' || piece == 'b') && to - from == 1)
            return true; // Move down one square for Black pawns

        // Moving horizontally
        if (from - to != 0) {
            char temp = board[from][from];
            board[from][from] = '\0';
            board[to][to] = temp;
            piece = temp;

            char target_piece2 = board[toy][tox];

            return (target_piece == '\0' || piece == target_piece); // If it is empty, then move can be legal
        }

        if (abs(to - from) != 1)
            return false; // Moving diagonally does not work

        char temp2 = board[toy][tox];
        board[toy][toy] = '\0';
        board[from][from] = temp2;

        if (piece == 'R')
            return ((abs(to - from) % 8 == 1 && toy + abs(to - from) >= rows));
        else
            return ((abs(to - from) % 8 == 7 && toy + abs(to - from) <= 0));

    }
}

// Function to check for king capture
bool has_king(bool king) {
    if (king)
        cout << "Black King is in danger." << endl;
    else
        cout << "Red King is in danger." << endl;

    return true;
}

// Function to get the side of player
bool get_side_choice() {
    // Display current state of board and count number of pieces for each color
    print_board();
    int red_pieces = 0;
    int black_pieces = 0;
    int red_kings = 0;
    int black_kings = 0;

    for (int i = 0; i < rows; i++) {
        if (board[i][i] != '\0')
            red_kings++;
        if (board[i][(rows - 1) - i] != '\0')
            black_kings++;

        if (board[i][i] == 'R') // Check for Red pieces
            red_pieces++;
        if (board[i][i] == 'B') // Check for Black pieces
            black_pieces++;

    }

    cout << "Red: ";
    for (int i = 0; i < red_kings + red_pieces / 2; i++)
        cout << "R ";
    cout << endl;
    cout << "Black: ";
    for (int i = 0; i < black_kings + black_pieces / 2; i++)
        cout << "B ";
    cout << endl;

    int side;
    cout << "Choose a side: " << endl;
    cin >> side;
    return side == 1;
}

// Function to print player move choice
void get_move_choice() {
    char move_from, move_to;
    bool is_legal = false;

    do {
        // Display current state of board and count number of pieces for each color
        print_board();

        cout << "Enter starting piece letter: ";
        cin >> move_from;
        cout << "Enter target piece letter: ";
        cin >> move_to;
        if (move_from > 'A' || move_from < 'A')
            cout << "Invalid move. Please try again." << endl;
        else
            is_legal = true;

    } while (!is_legal);
}

// Function to check for valid moves and apply them
void make_move() {
    int from, to, temp, tox, toy;
    char piece, target_piece;

    do {
        get_move_choice();
        // Get the starting position of move
        cout << "Enter row of first letter (0-7): ";
        cin >> fromy;
        if(fromy > 7 || fromy < 0)
            cout << "Invalid position. Please try again."<<endl;
        else {
            cout << "Enter column of first letter (0-7): ";
            cin >> fromx;
        }

        // Get the target position
        cout << "Enter row of second letter (0-7): ";
        cin >> toy;
        if(toy > 7 || toy < 0)
            cout << "Invalid position. Please try again."<<endl;
        else {
            cout << "Enter column of second letter (0-7): ";
            cin >> tox;
        }

        // Make the move
        from = fromx * 8 + fromy;
        to = tox * 8 + toy;

        if (is_legal_move(from, to))
        {
            piece = board[from][from];
            temp = board[to][to];

            board[from][from] = '\0';
            board[to][to] = temp;

            char target_piece2 = board[toy][tox];

            // Move the King to a new square
            if (piece == 'K') {
                if(to - from != 1)
                    return;
                else
                {
                    if((from + abs(to - from)) % 8 > 0)
                        toy += abs(to - from);
                    board[toy][toy] = '\0';
                    king = true;

                }
            }

            // Make the capture move
            if (target_piece2 != '\0') {

                char target_piece3 = board[capture_fromy][capture_tox];

                // Capture piece

                board[capture_fromy][capture_tox] = '\0';

                if (piece == 'R' || piece == 'r')
                    toy += abs(to - from);
                else
                    toy -= abs(to - from);

                if ((to - from) % 8 == 1)
                    board[toy][(toy + abs(to - from)) % 8] = '\0';
                else
                    board[(toy - abs(to - from)) % 8][toy] = '\0';

            }

        } // If the move is not legal, print a message

    } while (true);

}

// Function to draw the final board state
void draw_board() {
    int king;
    cout << "Enter your color for king ('r' or 'R'): ";
    cin >> king;

    if(king == 'r')
        king = true;
    else
        king = false;

    print_board();
}

int main() {
    // Initialize global variables
    rows = 8;
    cols = 8;
    moves = 0;
    board[3][3] = 'R';
    board[4][4] = 'B';
    board[2][6] = 'B';

    while (1) {
        if (!king)
            cout << "Red turn. Move a piece to make it a King."<<endl;
        else
            cout << "Black turn. Try to capture your opponent's pieces."<<endl;

        // Get player choice and apply the move

        get_side_choice();

        if (king) {
            draw_board();
            cout << "Red King is in danger." << endl;
        }

        make_move();

        moves++;
    }

    return 0;
}
