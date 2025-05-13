#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MQTT_BROKER "34.145.113.189"

int is_cell_free(const char *state, int cell) {
    return state[cell] == ' ';
}

void get_board_state(char *state) {
    FILE *fp = popen("mosquitto_sub -h " MQTT_BROKER " -t ttt/game/state -C 1", "r");
    if (fp) {
        fgets(state, 10, fp);
        pclose(fp);
    } else {
        strcpy(state, "         ");
    }
}

void get_status(char *status) {
    FILE *fp = popen("mosquitto_sub -h " MQTT_BROKER " -t ttt/game/status -C 1", "r");
    if (fp) {
        fgets(status, 64, fp);
        status[strcspn(status, "\r\n")] = 0;
        pclose(fp);
    } else {
        strcpy(status, "ERROR");
    }
}

void send_move(int cell) {
    char cmd[128];
    snprintf(cmd, sizeof(cmd),
             "mosquitto_pub -h " MQTT_BROKER " -t ttt/game/move/x -m \"%d\"", cell);
    system(cmd);
}
