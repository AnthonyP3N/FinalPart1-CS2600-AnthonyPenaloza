#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// WiFi credentials
const char* ssid = "wifi";
const char* password = "password";

// MQTT broker
const char* mqtt_server = "34.145.113.189";

// Topics
const char* topic_move_x = "ttt/game/move/x";
const char* topic_move_o = "ttt/game/move/o";
const char* topic_state  = "ttt/game/state";
const char* topic_status = "ttt/game/status";
const char* topic_reset  = "ttt/game/reset";

// LCD config
#define SDA 14
#define SCL 13
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Game state
char board[3][3];
char currentPlayer = 'X';
WiFiClient espClient;
PubSubClient client(espClient);

// Game stats
int winsX = 0, winsO = 0, draws = 0;
bool gameOver = false;

// GAME LOGIC

void initBoard() {
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      board[i][j] = ' ';
}

bool makeMove(int row, int col) {
  if (row < 0 || row > 2 || col < 0 || col > 2 || board[row][col] != ' ')
    return false;
  board[row][col] = currentPlayer;
  return true;
}

char checkWinner() {
  for (int i = 0; i < 3; i++) {
    if (board[i][0] != ' ' && board[i][0] == board[i][1] && board[i][1] == board[i][2])
      return board[i][0];
    if (board[0][i] != ' ' && board[0][i] == board[1][i] && board[1][i] == board[2][i])
      return board[0][i];
  }
  if (board[0][0] != ' ' && board[0][0] == board[1][1] && board[1][1] == board[2][2])
    return board[0][0];
  if (board[0][2] != ' ' && board[0][2] == board[1][1] && board[1][1] == board[2][0])
    return board[0][2];
  return ' ';
}

bool isBoardFull() {
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      if (board[i][j] == ' ')
        return false;
  return true;
}

void publishGameState() {
  String state = "";
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      state += board[i][j];
  client.publish(topic_state, state.c_str(), true);  // Retain
}

void publishGameStatus() {
  if (gameOver) return;

  char winner = checkWinner();

  if (winner != ' ') {
    gameOver = true;

    String result = "Winner: ";
    result += winner;
    client.publish(topic_status, result.c_str(), true);

    if (winner == 'X') winsX++;
    else if (winner == 'O') winsO++;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.printf("XW:%d L:%d D:%d", winsX, winsO, draws);
    lcd.setCursor(0, 1);
    lcd.printf("OW:%d L:%d D:%d", winsO, winsX, draws);
  }
  else if (isBoardFull()) {
    gameOver = true;

    client.publish(topic_status, "Draw", true);
    draws++;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.printf("XW:%d L:%d D:%d", winsX, winsO, draws);
    lcd.setCursor(0, 1);
    lcd.printf("OW:%d L:%d D:%d", winsO, winsX, draws);
  }
  else {
    String next = "Next: ";
    next += currentPlayer;
    client.publish(topic_status, next.c_str(), true);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.printf("Next: %c", currentPlayer);
  }
}

void resetGame() {
  initBoard();
  currentPlayer = 'X';
  gameOver = false;
  publishGameState();
  publishGameStatus();
}

void handleMove(char player, int cellIndex) {
  int row = cellIndex / 3;
  int col = cellIndex % 3;

  if (makeMove(row, col)) {
    publishGameState();
    currentPlayer = (player == 'X') ? 'O' : 'X';
    publishGameStatus();
  }
}

// MQTT 

void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  int cell = atoi((char*)payload);

  if (strcmp(topic, topic_move_x) == 0 && currentPlayer == 'X') {
    handleMove('X', cell);
  } else if (strcmp(topic, topic_move_o) == 0 && currentPlayer == 'O') {
    handleMove('O', cell);
  } else if (strcmp(topic, topic_reset) == 0) {
    resetGame();
  }
}

void setup_wifi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32Client")) {
      client.subscribe(topic_move_x);
      client.subscribe(topic_move_o);
      client.subscribe(topic_reset);
      publishGameState();
      publishGameStatus();
    } else {
      delay(1000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  initBoard();

  Wire.begin(SDA, SCL);
  lcd.init();
  lcd.backlight();

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  reconnect();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  client.loop();

  static unsigned long lastStatusTime = 0;
  if (millis() - lastStatusTime > 5000 && !gameOver) {
    publishGameStatus();
    lastStatusTime = millis();
  }
}
