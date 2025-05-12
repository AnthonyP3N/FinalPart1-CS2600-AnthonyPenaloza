#ifndef TICTACTOE_H
#define TICTACTOE_H

#include <stdbool.h>

//Makes it so ESP32 complies with C files properly, without it creates issues
#ifdef __cplusplus
extern "C" {
#endif

extern char board[3][3];
extern char currentPlayer;

void initBoard();
char checkWinner();
bool isBoardFull();
bool makeMove(int row, int col);
void resetGame();

#ifdef __cplusplus
}
#endif

#endif
