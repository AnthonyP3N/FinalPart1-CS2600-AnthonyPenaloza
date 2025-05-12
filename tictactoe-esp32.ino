#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "tictactoe.h"

#define SDA 14
#define SCL 13

LiquidCrystal_I2C lcd(0x27, 16, 2);

bool i2CAddrTest(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

String inputBuffer = "";
bool gameOver = false;
bool askingReplay = false;

int gamesPlayed = 0;
int winsX = 0;
int lossesX = 0;
int winsO = 0;
int lossesO = 0;
int draws = 0;

void printBoard() {
    Serial.println();
    for (int i = 0; i < 3; i++) {
        Serial.printf(" %c | %c | %c \n", board[i][0], board[i][1], board[i][2]);
        if (i < 2) Serial.println("---|---|---");
    }
    Serial.println();
}

void promptMove() {
    Serial.printf("Player %c, enter your move (row and col: 0-2 0-2): ", currentPlayer);
}

void updateLCD() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.printf("XW:%d L:%d D:%d", winsX, lossesX, draws);
    lcd.setCursor(0, 1);
    lcd.printf("OW:%d L:%d D:%d", winsO, lossesO, draws);
}

void setup() {
    Serial.begin(115200);
    Wire.begin(SDA, SCL);
    if (!i2CAddrTest(0x27)) {
        lcd = LiquidCrystal_I2C(0x3F, 16, 2);
    }
    lcd.init();
    lcd.backlight();
    delay(1000);
    resetGame();
    printBoard();
    updateLCD();
    promptMove();
}

void loop() {
    if (Serial.available()) {
        inputBuffer = Serial.readStringUntil('\n');
        inputBuffer.trim();

        if (gameOver && askingReplay) {
            if (inputBuffer.equalsIgnoreCase("y")) {
                resetGame();
                printBoard();
                updateLCD();
                promptMove();
                gameOver = false;
                askingReplay = false;
            } else {
                Serial.println("Thanks for playing!");
                while (true);
            }
            return;
        }

        if (!gameOver) {
            int row, col;
            if (sscanf(inputBuffer.c_str(), "%d %d", &row, &col) == 2) {
                if (makeMove(row, col)) {
                    Serial.printf("Player %c placed a mark at row %d, column %d\n", currentPlayer, row, col);
                    printBoard();

                    char winner = checkWinner();
                    if (winner != ' ') {
                        Serial.printf("Player %c wins!\n", winner);
                        gameOver = true;
                        gamesPlayed++;
                        if (winner == 'X') {
                            winsX++;
                            lossesO++;
                        } else {
                            winsO++;
                            lossesX++;
                        }
                        updateLCD();
                        Serial.print("Play again? (y/n): ");
                        askingReplay = true;
                    } else if (isBoardFull()) {
                        Serial.println("It's a draw!");
                        gameOver = true;
                        gamesPlayed++;
                        draws++;
                        updateLCD();
                        Serial.print("Play again? (y/n): ");
                        askingReplay = true;
                    } else {
                        currentPlayer = (currentPlayer == 'X') ? 'O' : 'X';
                        promptMove();
                    }
                } else {
                    Serial.println("Invalid move. Try again.");
                    promptMove();
                }
            } else {
                Serial.println("Invalid input. Use: row col");
                promptMove();
            }
        }
    }
}
