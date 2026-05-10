/*
 Jonathon Tucker
 Final 411  
 */

#include <iostream>
#include <array>
#include <limits>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <random>
#include <algorithm>

// ── Types ─────────────────────────────────────────────────────────────────────

using Board = std::array<char, 9>;  // ' ', 'X', or 'O'

struct Result {
    char winner;   // 'X', 'O', 'D' (draw), or '\0' (no result yet)
    std::array<int, 3> line;
};

// ── ANSI colors ──────────────────────────────────────────────────────────────

namespace Color {
    const std::string RESET   = "\033[0m";
    const std::string BOLD    = "\033[1m";
    const std::string X_COL   = "\033[96m";   // cyan  – X
    const std::string O_COL   = "\033[38;5;208m"; // orange – O
    const std::string WIN_COL = "\033[93m";   // yellow – highlights
    const std::string DIM     = "\033[38;5;240m";
    const std::string TITLE   = "\033[38;5;214m";
}

// ── Game logic ────────────────────────────────────────────────────────────────

Result checkWinner(const Board& board) {
    constexpr int lines[8][3] = {
        {0,1,2},{3,4,5},{6,7,8},
        {0,3,6},{1,4,7},{2,5,8},
        {0,4,8},{2,4,6}
    };
    for (auto& l : lines) {
        if (board[l[0]] != ' ' &&
            board[l[0]] == board[l[1]] &&
            board[l[0]] == board[l[2]])
            return { board[l[0]], {l[0], l[1], l[2]} };
    }
    for (char c : board)
        if (c == ' ') return { '\0', {-1,-1,-1} };
    return { 'D', {-1,-1,-1} };
}

int minimax(Board board, bool isMaximizing, int alpha, int beta) {
    Result res = checkWinner(board);
    if (res.winner == 'X') return -10;
    if (res.winner == 'O') return  10;
    if (res.winner == 'D') return   0;

    if (isMaximizing) {
        int best = std::numeric_limits<int>::min();
        for (int i = 0; i < 9; ++i) {
            if (board[i] == ' ') {
                board[i] = 'O';
                best = std::max(best, minimax(board, false, alpha, beta));
                board[i] = ' ';
                alpha = std::max(alpha, best);
                if (beta <= alpha) break;
            }
        }
        return best;
    } else {
        int best = std::numeric_limits<int>::max();
        for (int i = 0; i < 9; ++i) {
            if (board[i] == ' ') {
                board[i] = 'X';
                best = std::min(best, minimax(board, true, alpha, beta));
                board[i] = ' ';
                beta = std::min(beta, best);
                if (beta <= alpha) break;
            }
        }
        return best;
    }
}

int getBestMove(Board board) {
    int bestVal = std::numeric_limits<int>::min();
    int bestMove = -1;
    for (int i = 0; i < 9; ++i) {
        if (board[i] == ' ') {
            board[i] = 'O';
            int val = minimax(board, false,
                              std::numeric_limits<int>::min(),
                              std::numeric_limits<int>::max());
            board[i] = ' ';
            if (val > bestVal) { bestVal = val; bestMove = i; }
        }
    }
    return bestMove;
}

int getRandomMove(const Board& board) {
    std::vector<int> empty;
    for (int i = 0; i < 9; ++i)
        if (board[i] == ' ') empty.push_back(i);
    if (empty.empty()) return -1;
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, (int)empty.size() - 1);
    return empty[dist(rng)];
}

// ── Clear screen ───────────────────────────────────────────────────────────

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    std::cout << "\033[2J\033[H";
#endif
}

void printTitle() {}

std::string cellStr(char c, int idx,
                    const std::array<int,3>& winLine, int lastMove) {
    bool isWin  = (winLine[0]==idx || winLine[1]==idx || winLine[2]==idx);
    bool isLast = (lastMove == idx);

    if (c == 'X') {
        std::string col = isWin ? Color::WIN_COL : Color::X_COL;
        return col + Color::BOLD + " X " + Color::RESET;
    }
    if (c == 'O') {
        std::string col = isWin ? Color::WIN_COL : Color::O_COL;
        return col + Color::BOLD + " O " + Color::RESET;
    }
    // Empty – show position hint in dim colour
    (void)isLast;
    return Color::DIM + " " + std::to_string(idx + 1) + " " + Color::RESET;
}

void printBoard(const Board& board,
                const std::array<int,3>& winLine, int lastMove) {
    std::cout << "\n";
    for (int row = 0; row < 3; ++row) {
        std::cout << "  ";
        for (int col = 0; col < 3; ++col) {
            int i = row * 3 + col;
            std::cout << cellStr(board[i], i, winLine, lastMove);
            if (col < 2) std::cout << Color::DIM << "│" << Color::RESET;
        }
        std::cout << "\n";
        if (row < 2)
            std::cout << Color::DIM << "  ───┼───┼───\n" << Color::RESET;
    }
    std::cout << "\n";
}

void printScores(int xScore, int oScore, int draws, const std::string& oLabel) {
    std::cout << Color::DIM << "  ┌──────────────────────────┐\n"
              << "  │  "
              << Color::X_COL  << "You (X) "
              << Color::DIM    << " Draw  "
              << Color::O_COL  << oLabel
              << Color::DIM    << " │\n"
              << "  │  "
              << Color::X_COL  << Color::BOLD << "  " << xScore << "    "
              << Color::RESET  << Color::DIM  << "   " << draws << "    "
              << Color::O_COL  << Color::BOLD << "  " << oScore << "    "
              << Color::DIM    << "  │\n"
              << "  └──────────────────────────┘\n"
              << Color::RESET << "\n";
}

// ── Main game loop ────────────────────────────────────────────────────────────

int main() {
    // 0 = minimax AI, 1 = random agent
    int  mode    = 0;
    int  xScore  = 0, oScore = 0, draws = 0;

    // Mode selection
    clearScreen();
    printTitle();
    std::cout << "  Select opponent:\n"
              << "    " << Color::WIN_COL << "[1]" << Color::RESET
              << " Minimax AI" << Color::DIM << "  (alpha-beta pruning)\n" << Color::RESET
              << "    " << Color::WIN_COL << "[2]" << Color::RESET
              << " Random Agent" << Color::DIM << "  (random moves)\n\n" << Color::RESET
              << "  Choice: ";
    char choice;
    std::cin >> choice;
    mode = (choice == '2') ? 1 : 0;

    const std::string oLabel = (mode == 0) ? "Minimax " : "Random  ";

    while (true) {
        Board board;
        board.fill(' ');
        bool xTurn    = true;
        int  lastMove = -1;
        std::array<int,3> winLine = {-1,-1,-1};

        while (true) {
            Result res = checkWinner(board);
            clearScreen();
            printTitle();
            printScores(xScore, oScore, draws, oLabel);

            if (res.winner != '\0') winLine = res.line;
            printBoard(board, winLine, lastMove);

            // Status line
            if (res.winner != '\0') {
                if (res.winner == 'D')
                    std::cout << Color::WIN_COL << "  It's a draw!\n" << Color::RESET;
                else if (res.winner == 'X')
                    std::cout << Color::WIN_COL << "  You win!\n" << Color::RESET;
                else
                    std::cout << Color::WIN_COL << "  " << oLabel << "wins!\n" << Color::RESET;

                if (res.winner == 'X') ++xScore;
                else if (res.winner == 'O') ++oScore;
                else ++draws;

                std::cout << "\n  Play again? [y/n]: ";
                char again; std::cin >> again;
                if (again != 'y' && again != 'Y') { std::cout << "\n"; return 0; }
                break;
            }

            // Opponent (O) turn
            if (!xTurn) {
                int move = -1;
                if (mode == 0) {
                    // Minimax
                    //std::cout << Color::O_COL << "  Minimax thinking…" << Color::RESET << std::flush;
                    std::this_thread::sleep_for(std::chrono::milliseconds(400));
                    move = getBestMove(board);
                } else {
                    // Random agent
                    //std::cout << Color::O_COL << "  Random agent choosing…" << Color::RESET << std::flush;
                    std::this_thread::sleep_for(std::chrono::milliseconds(350));
                    move = getRandomMove(board);
                }
                if (move != -1) {
                    board[move] = 'O';
                    lastMove = move;
                }
                xTurn = true;
                continue;
            }

            // Human (X) turn
            std::cout << Color::X_COL << "  Your turn (X)" << Color::RESET
                      << Color::DIM << " – enter position [1-9]: " << Color::RESET;

            int pos;
            if (!(std::cin >> pos) || pos < 1 || pos > 9) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                continue;
            }
            int idx = pos - 1;
            if (board[idx] != ' ') {
                std::cout << Color::DIM << "  Cell taken, try again.\n" << Color::RESET;
                std::this_thread::sleep_for(std::chrono::milliseconds(700));
                continue;
            }
            board[idx] = 'X';
            lastMove   = idx;
            xTurn      = false;
        }
    }
}
