#include <iostream>
#include <vector>
#include <random>
#include <algorithm>

// Define piece values
const int PAWN = 1;
const int KNIGHT = 3;
const int BISHOP = 3;
const int ROOK = 5;
const int QUEEN = 9;
const int KING = 100;

// Function to initialize the board and set up starting positions for each player
void initialize_game_state() {
    // Initialize a standard chess board with all pieces in their starting positions
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            if (i == 1) {
                if (j % 2 == 0) {
                    board[j * 8 + i] = 'r'; // White rook
                } else {
                    board[j * 8 + i] = 'R'; // Black rook
                }
            } else if (i == 6) {
                if (j % 2 == 0) {
                    board[j * 8 + i] = 'n'; // White knight
                } else {
                    board[j * 8 + i] = 'N'; // Black knight
                }
            } else if (i == 7 || i == 3) {
                if (j % 2 == 0) {
                    board[j * 8 + i] = 'b'; // White bishop
                } else {
                    board[j * 8 + i] = 'B'; // Black bishop
                }
            } else if (i == 2 || i == 5) {
                if (j % 2 == 0) {
                    board[j * 8 + i] = 'q'; // White queen
                } else {
                    board[j * 8 + i] = 'Q'; // Black queen
                }
            } else if (i == 0 || i == 7) {
                if (j % 2 == 0) {
                    board[j * 8 + i] = 'k'; // White king
                } else {
                    board[j * 8 + i] = 'K'; // Black king
                }
            } else if (i == 1 || i == 6) {
                if (j % 2 == 0) {
                    board[j * 8 + i] = 'p'; // White pawn
                } else {
                    board[j * 8 + i] = 'P'; // Black pawn
                }
            }
        }
    }

    // Make sure white is not playing as black
    if (side == "b") {
        std::cout << "Invalid move. You can't be black and play as black at the same time." << std::endl;
        return;
    }
}

// Function to print the welcome message
void print_welcome() {
    std::cout << "Welcome to Chess Game!" << std::endl;
    std::cout << "This is a simple implementation of chess in C++." << std::endl;
    std::cout << "Please choose a side (white or black) when prompted." << std::endl;
}

// Function to get the player's side
void get_side_choice() {
    char choice;
    std::cin >> choice;

    while (choice != 'w' && choice != 'b') {
        std::cout << "Invalid move. Please choose white or black (w or b)." << std::endl;
        std::cin >> choice;
    }

    if (choice == 'w') {
        side = true;
    } else {
        side = false;
    }
}

// Function to print the board
void print_board() {
    for (int i = 0; i < 8; ++i) {
        std::cout << i + 1 << " ";
        for (int j = 0; j < 8; ++j) {
            std::cout << board[j * 8 + i] << " ";
        }
        std::cout << std::endl;
    }
}

// Function to initialize the current move
void get_move_choice() {
    static int turn = 1;

    if (turn % 2 == 0 && side) { // White's turn
        print_board();
        std::cin >> from >> to; // Get the move from user

        if (!is_legal_move(from, to)) { // Check if the move is valid
            std::cout << "Invalid move." << std::endl;
            return;
        }

        char piece = board[from];
        int score;

        // Evaluate the move using a chess engine like alpha-beta pruning
        // For simplicity, we'll use a basic evaluation function for now.
        if (piece == 'p') {
            score = 1; // Pawn value
        } else if (piece == 'n' || piece == 'b' || piece == 'q' || piece == 'r' || piece == 'k') {
            score = PAWN;
        }

        if (score > 0) { // White moves
            board[to] = piece; // Update the board with the move

            if (!has_king(side)) { // Check for checkmate
                std::cout << "Black wins." << std::endl;
                return;
            }
        } else if (score < 0) { // Black moves
            board[from] = piece; // Update the board with the move

            if (!has_king(!side)) { // Check for checkmate
                std::cout << "White wins." << std::endl;
                return;
            }
        } else {
            std::cout << "Invalid move. No scoring possible." << std::endl;
            return;
        }

        turn++; // Move to the opponent's turn

        while (turn % 2 == 0 && side) { // White's turn
            print_board();
            std::cin >> from >> to; // Get the move from user

            if (!is_legal_move(from, to)) { // Check if the move is valid
                std::cout << "Invalid move." << std::endl;
                return;
            }

            char piece = board[from];
            int score;

            if (piece == 'p') {
                score = 1; // Pawn value
            } else if (piece == 'n' || piece == 'b' || piece == 'q' || piece == 'r' || piece == 'k') {
                score = PAWN;
            }

            if (score > 0) { // White moves
                board[to] = piece; // Update the board with the move

                if (!has_king(side)) { // Check for checkmate
                    std::cout << "Black wins." << std::endl;
                    return;
                }
            } else if (score < 0) { // Black moves
                board[from] = piece; // Update the board with the move

                if (!has_king(!side)) { // Check for checkmate
                    std::cout << "White wins." << std::endl;
                    return;
                }
            } else {
                std::cout << "Invalid move. No scoring possible." << std::endl;
                return;
            }

            turn++; // Move to the opponent's turn
        }
    } else { // Black's turn
        print_board();
        std::cin >> from >> to; // Get the move from user

        if (!is_legal_move(from, to)) { // Check if the move is valid
            std::cout << "Invalid move." << std::endl;
            return;
        }

        char piece = board[from];
        int score;

        if (piece == 'p') {
            score = -1; // Pawn value
        } else if (piece == 'n' || piece == 'b' || piece == 'q' || piece == 'r' || piece == 'k') {
            score = PAWN;
        }

        if (score > 0) { // Black moves
            board[to] = piece; // Update the board with the move

            if (!has_king(!side)) { // Check for checkmate
                std::cout << "White wins." << std::endl;
                return;
            }
        } else if (score < 0) { // White moves
            board[from] = piece; // Update the board with the move

            if (!has_king(side)) { // Check for checkmate
                std::cout << "Black wins." << std::endl;
                return;
            }
        } else {
            std::cout << "Invalid move. No scoring possible." << std::endl;
            return;
        }

        turn++; // Move to the opponent's turn

        while (turn % 2 == 0 && !side) { // White's turn
            print_board();
            std::cin >> from >> to; // Get the move from user

            if (!is_legal_move(from, to)) { // Check if the move is valid
                std::cout << "Invalid move." << std::endl;
                return;
            }

            char piece = board[from];
            int score;

            if (piece == 'p') {
                score = 1; // Pawn value
            } else if (piece == 'n' || piece == 'b' || piece == 'q' || piece == 'r' || piece == 'k') {
                score = PAWN;
            }

            if (score > 0) { // White moves
                board[to] = piece; // Update the board with the move

                if (!has_king(side)) { // Check for checkmate
                    std::cout << "Black wins." << std::endl;
                    return;
                }
            } else if (score < 0) { // Black moves
                board[from] = piece; // Update the board with the move

                if (!has_king(!side)) { // Check for checkmate
                    std::cout << "White wins." << std::endl;
                    return;
                }
            } else {
                std::cout << "Invalid move. No scoring possible." << std::endl;
                return;
            }

            turn++; // Move to the opponent's turn
        }
    }
}

// Function to evaluate if a piece is on the board and the turn belongs to the player
bool is_piece_on_board(int from, int to) {
    return from >= 0 && from < 64 && to >= 0 && to < 64;
}

// Function to check if a move is valid (within the allowed number of squares)
bool is_legal_move(int from, int to) {
    // Simple implementation for demonstration purposes
    // In a real game of chess, this would involve more complex logic to handle different types of pieces and movements.
    return abs(from - to) <= 1;
}

// Function to check if there's a king on the board that can be captured with the current move
bool has_king(bool side) {
    int index = (side ? 0 : 63);
    bool found = false;

    for (int i = 0; i < 8; ++i) {
        if (board[index + i * 8] == 'k' || board[index - i * 8] == 'K') {
            found = true;
            break;
        }
    }

    return found;
}

// Function to check if a move would put the opponent's king in check
bool is_check(bool side) {
    int index = (side ? 0 : 63);

    for (int i = 0; i < 8; ++i) {
        char piece = board[index + i * 8];
        bool found = false;

        if (piece == 'p') { // Pawn
            for (int j = i - 1; j >= 0; --j) {
                if (board[index + j * 8] != '.' && board[index + j * 8] != piece) {
                    break;
                }

                found = true;
                break;
            }
        } else if (piece == 'n' || piece == 'b' || piece == 'q' || piece == 'r') { // Knight, Bishop, Queen, Rook
            bool valid_move = false;

            for (int j = i - 1; j >= 0; --j) {
                if (board[index + j * 8] != '.' && board[index + j * 8] != piece) {
                    break;
                }

                found = true;
                for (int k = 0; k < 8; ++k) {
                    if (abs(i - k) == abs(j - i)) {
                        valid_move = true;
                        break;
                    }
                }

                if (valid_move) {
                    break;
                }
            }

            found = false;

            for (int j = i + 1; j < 8; ++j) {
                if (board[index + j * 8] != '.' && board[index + j * 8] != piece) {
                    break;
                }

                found = true;
                for (int k = 0; k < 8; ++k) {
                    if (abs(i - k) == abs(j - i)) {
                        valid_move = true;
                        break;
                    }
                }

                if (valid_move) {
                    break;
                }
            }
        } else { // King
            found = true;
        }

        if (!found) {
            break;
        }
    }

    return found;
}

int main() {
    initialize_game_state();

    while (true) {
        get_side_choice();
        print_board();
        std::cin >> from >> to; // Get the move from user
        if (!is_legal_move(from, to)) { // Check if the move is valid
            std::cout << "Invalid move." << std::endl;
            continue;
        }

        char piece = board[from];
        int score;

        if (piece == 'p') {
            score = 1; // Pawn value
        } else if (piece == 'n' || piece == 'b' || piece == 'q' || piece == 'r' || piece == 'k') {
            score = PAWN;
        }

        if (score > 0) { // White moves
            board[to] = piece; // Update the board with the move

            if (!has_king(true)) { // Check for checkmate
                std::cout << "Black wins." << std::endl;
                return 0;
            }
        } else if (score < 0) { // Black moves
            board[from] = piece; // Update the board with the move

            if (!has_king(false)) { // Check for checkmate
                std::cout << "White wins." << std::endl;
                return 0;
            }
        } else {
            std::cout << "Invalid move. No scoring possible." << std::endl;
            continue;
        }

        bool found = false;

        for (int i = 1; i <= 8; ++i) {
            if (is_piece_on_board(from, to)) {
                char p = board[to];

                // Check if the square is occupied by an opponent's piece
                if (!found && (p == 'w' || p == 'b')) {
                    found = true;
                }

                break;
            }
        }

        if (!found) {
            std::cout << "Move would put your king in check!" << std::endl;
            continue;
        } else { // Opponent's move
            board[from] = piece; // Update the board with the move

            for (int i = 1; i <= 8; ++i) {
                if (is_piece_on_board(from, to)) {
                    char p = board[to];

                    // Check if the square is occupied by an opponent's piece
                    if (!found && (p == 'w' || p == 'b')) {
                        found = true;
                        break;
                    }
                }
            }

            if (!found) {
                std::cout << "You can't move your king!" << std::endl;
                continue;
            } else { // Your opponent's turn
                board[to] = piece; // Update the board with the move

                for (int i = 1; i <= 8; ++i) {
                    if (is_piece_on_board(from, to)) {
                        char p = board[to];

                        // Check if the square is occupied by an opponent's piece
                        if (!found && (p == 'w' || p == 'b')) {
                            found = true;
                            break;
                        }
                    }
                }

                continue;
            }
        }
    }

    return 0;
}
