#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>  // For Sleep()
#include <ctype.h>    // For tolower()

#define MQTT_BROKER "34.145.113.189"

int wins = 0, losses = 0, draws = 0;

void draw_board(const char *state) {
    printf("\n");
    for (int i = 0; i < 9; i++) {
        char cell = (state[i] == ' ') ? '1' + i : state[i];
        printf(" %c ", cell);
        if (i % 3 != 2) printf("|");
        else if (i != 8) printf("\n-----------\n");
    }
    printf("\n");
}

void get_board_state(char *state) {
    FILE *fp = popen("mosquitto_sub -h " MQTT_BROKER " -t ttt/game/state -C 1", "r");
    if (!fp) {
        printf("Error reading board state.\n");
        strcpy(state, "         ");
        return;
    }
    fgets(state, 10, fp);
    pclose(fp);
}

void get_game_status(char *status) {
    FILE *fp = popen("mosquitto_sub -h " MQTT_BROKER " -t ttt/game/status -C 1", "r");
    if (!fp) {
        strcpy(status, "ERROR: couldn't read");
        return;
    }
    if (fgets(status, 64, fp) != NULL) {
        status[strcspn(status, "\r\n")] = 0;
    } else {
        strcpy(status, "ERROR: no data received");
    }
    pclose(fp);
}

void send_move(int cell, char player) {
    char cmd[128];
    snprintf(cmd, sizeof(cmd),
             "mosquitto_pub -h " MQTT_BROKER " -t ttt/game/move/%c -m \"%d\"",
             tolower(player), cell);
    system(cmd);
}

void send_reset() {
    char cmd[128];
    snprintf(cmd, sizeof(cmd),
             "mosquitto_pub -h " MQTT_BROKER " -t ttt/game/reset -m \"reset\"");
    system(cmd);
}

void wait_for_reset_to_take_effect() {
    char status[64];
    while (1) {
        Sleep(1000);
        get_game_status(status);
        if (!strstr(status, "Winner:") && strcmp(status, "Draw") != 0)
            break;
    }
    Sleep(500);
}

void play_one_player_mode() {
    char state[16] = "         ";
    char status[64];
    int move;

    printf("1P Mode: You are X. Bash bot is O.\n");

    while (1) {
        get_board_state(state);
        draw_board(state);
        get_game_status(status);

        printf("Status: %s\n", status);
        printf("Score: W:%d L:%d D:%d\n", wins, losses, draws);

        if (strstr(status, "Winner:")) {
            if (strstr(status, "X")) {
                printf("You won!\n");
                wins++;
            } else {
                printf("You lost!\n");
                losses++;
            }

            printf("Play again? (y/n): ");
            char choice = getchar(); getchar();
            if (choice == 'y' || choice == 'Y') {
                send_reset();
                wait_for_reset_to_take_effect();
                continue;
            } else break;
        } else if (strcmp(status, "Draw") == 0) {
            printf("Game ended in a draw.\n");
            draws++;

            printf("Play again? (y/n): ");
            char choice = getchar(); getchar();
            if (choice == 'y' || choice == 'Y') {
                send_reset();
                wait_for_reset_to_take_effect();
                continue;
            } else break;
        }

        if (strcmp(status, "Next: X") == 0) {
            printf("Enter your move (1-9): ");
            scanf("%d", &move);
            getchar();
            move--;

            if (move < 0 || move > 8 || state[move] != ' ') {
                printf("Invalid move. Press Enter to try again.\n");
                getchar();
                continue;
            }

            send_move(move, 'X');
            Sleep(1000);
        } else {
            char new_status[64];
            do {
                Sleep(1500);
                get_game_status(new_status);
            } while (strcmp(new_status, status) == 0);

            strcpy(status, new_status);
        }
    }

    printf("\nFinal Score - Wins: %d | Losses: %d | Draws: %d\n", wins, losses, draws);
}

void play_two_player_mode() {
    char state[16] = "         ";
    char status[64];
    int move;
    char current = 'X';

    int winsX_local = 0, winsO_local = 0, draws_local = 0;

    printf("2P Mode: Both players use this terminal.\n");

    while (1) {
        get_board_state(state);
        draw_board(state);
        get_game_status(status);

        printf("Status: %s\n", status);
        printf("Score: X Wins: %d | O Wins: %d | Draws: %d\n", winsX_local, winsO_local, draws_local);

        if (strstr(status, "Winner: X")) {
            printf("Player X wins!\n");
            winsX_local++;

            printf("Play again? (y/n): ");
            char choice = getchar(); getchar();
            if (choice == 'y' || choice == 'Y') {
                send_reset();
                wait_for_reset_to_take_effect();
                current = 'X';
                continue;
            } else break;

        } else if (strstr(status, "Winner: O")) {
            printf("Player O wins!\n");
            winsO_local++;

            printf("Play again? (y/n): ");
            char choice = getchar(); getchar();
            if (choice == 'y' || choice == 'Y') {
                send_reset();
                wait_for_reset_to_take_effect();
                current = 'X';
                continue;
            } else break;

        } else if (strcmp(status, "Draw") == 0) {
            printf("It's a draw!\n");
            draws_local++;

            printf("Play again? (y/n): ");
            char choice = getchar(); getchar();
            if (choice == 'y' || choice == 'Y') {
                send_reset();
                wait_for_reset_to_take_effect();
                current = 'X';
                continue;
            } else break;
        }

        if ((strcmp(status, "Next: X") == 0 && current == 'X') ||
            (strcmp(status, "Next: O") == 0 && current == 'O')) {
            printf("Player %c, enter your move (1-9): ", current);
            scanf("%d", &move);
            getchar();
            move--;

            if (move < 0 || move > 8 || state[move] != ' ') {
                printf("Invalid move. Press Enter to try again.\n");
                getchar();
                continue;
            }

            send_move(move, current);
            Sleep(1000);
            current = (current == 'X') ? 'O' : 'X';
        } else {
            Sleep(500);
        }
    }

    printf("\nFinal 2P Score - Player X: %d | Player O: %d | Draws: %d\n", winsX_local, winsO_local, draws_local);
}

int main() {
    int choice;

    while (1) {
        printf("\n=== Tic-Tac-Toe Menu ===\n");
        printf("1. Play 1-Player (vs Bash Bot)\n");
        printf("2. Play 2-Player (same laptop)\n");
        printf("3. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        getchar(); // flush newline

        switch (choice) {
            case 1:
                play_one_player_mode();
                break;
            case 2:
                play_two_player_mode();
                break;
            case 3:
                printf("Goodbye!\n");
                return 0;
            default:
                printf("Invalid choice. Try again.\n");
        }
    }

    return 0;
}
