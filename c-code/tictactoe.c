#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>  // For Sleep()

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

void send_move(int cell) {
    char cmd[128];
    snprintf(cmd, sizeof(cmd),
             "mosquitto_pub -h " MQTT_BROKER " -t ttt/game/move/x -m \"%d\"", cell);
    system(cmd);
}

void send_reset() {
    char cmd[128];
    snprintf(cmd, sizeof(cmd),
             "mosquitto_pub -h " MQTT_BROKER " -t ttt/game/reset -m \"reset\"");
    system(cmd);
}

int main() {
    char state[16] = "         ";
    char status[64];
    int move;

    printf("Welcome to MQTT Tic-Tac-Toe (Player X)\n");
    printf("Connecting to broker: %s\n", MQTT_BROKER);

    while (1) {
        printf("\n------------------------\n");

        get_board_state(state);
        draw_board(state);
        get_game_status(status);

        printf("Status: %s\n", status);
        printf("Score: W:%d L:%d D:%d\n", wins, losses, draws);

        // Game end condition
        if (strstr(status, "Winner:")) {
            if (strstr(status, "X")) {
                printf("You won!\n");
                wins++;
            } else {
                printf("You lost!\n");
                losses++;
            }

            printf("Play again? (y/n): ");
            char choice = getchar();
            getchar(); // consume newline
            if (choice == 'y' || choice == 'Y') {
                send_reset();
                Sleep(1500);
                continue;
            } else {
                break;
            }

        } else if (strcmp(status, "Draw") == 0) {
            printf("Game ended in a draw.\n");
            draws++;

            printf("Play again? (y/n): ");
            char choice = getchar();
            getchar(); // consume newline
            if (choice == 'y' || choice == 'Y') {
                send_reset();
                Sleep(1500);
                continue;
            } else {
                break;
            }
        }

        // Player X turn
        if (strcmp(status, "Next: X") == 0) {
            printf("Enter your move (1-9): ");
            scanf("%d", &move);
            getchar();
            move -= 1; //Was originally design for a grid of 0-8 but got confusing so 1-9 now, this just makes it so input is valid

            if (move < 0 || move > 8 || state[move] != ' ') {
                printf("Invalid move. Press Enter to try again.\n");
                getchar();
                continue;
            }

            send_move(move);
            Sleep(1000);  // Let ESP32 update MQTT

            get_board_state(state);
            get_game_status(status);
            draw_board(state);
            printf("Move sent. Status: %s\n", status);
        }
        // Waiting for Player O
        else {
            char new_status[64];
            do {
                Sleep(1500);
                get_game_status(new_status);
            } while (strcmp(new_status, status) == 0);

            strcpy(status, new_status);
            get_board_state(state);
            draw_board(state);
            printf("Status: %s\n", status);
        }
    }

    printf("\nFinal Score - Wins: %d | Losses: %d | Draws: %d\n", wins, losses, draws);
    return 0;
}
