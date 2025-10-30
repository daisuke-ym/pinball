#include <LiquidCrystal.h>
#include <mcp_can.h>
#include <SPI.h>
#include "def-vars.h"
#include "def-can.h"

const byte LCD_RS = A0, LCD_RW = A1, LCD_E = A2, LCD_D4 = A3, LCD_D5 = A4, LCD_D6 = A5, LCD_D7 = A6;
LiquidCrystal LCD(LCD_RS, LCD_RW, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

// CAN関係
#define CAN0_INT 37
MCP_CAN CAN0(53);
unsigned long rxId;
unsigned char rxLen = 0;
unsigned char rxBuf[8];
byte data[3] = {0x00, 0x00, 0x00};

// LCD用カスタム文字
byte customChar0[] = { // 短いマイナス
  0b00000,
  0b00000,
  0b00000,
  0b01110,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};
byte customChar1[] = { // 短いバックスラッシュ
  0b00000,
  0b00000,
  0b01000,
  0b00100,
  0b00010,
  0b00000,
  0b00000,
  0b00000
};
byte customChar2[] = { // 短い縦棒
  0b00000,
  0b00000,
  0b00100,
  0b00100,
  0b00100,
  0b00000,
  0b00000,
  0b00000
};
byte customChar3[] = { // 短いスラッシュ
  0b00000,
  0b00000,
  0b00010,
  0b00100,
  0b01000,
  0b00000,
  0b00000,
  0b00000
};

// テレメトリの進捗表示用文字
byte progressChar[] = {
  0, 1, 2, 3
};
byte progressLen = sizeof(progressChar) / sizeof(byte);

// ----------------------------------------------------------------------
void setup() {
	// Serial初期化
	Serial.begin(115200);
	// LCD初期化
	LCD.begin(20, 4);
  LCD.createChar(0, customChar0);
  LCD.createChar(1, customChar1);
  LCD.createChar(2, customChar2);
  LCD.createChar(3, customChar3);
	// CAN初期化
  if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK) {
    Serial.println("MCP2515 Initialized Successfully!");
    LCD.clear();
    LCD.setCursor(0, 0);
    LCD.print("CAN Init OK!");
    delay(1000);
  	disp_lcd_header();
  }
  else {
    Serial.println("Error Initializing MCP2515...");
    LCD.clear();
    LCD.setCursor(0, 0);
    LCD.print("CAN Error");
    while (1) {
      // nop
    }
  }
  CAN0.setMode(MCP_NORMAL);  // Set operation mode to normal so the MCP2515 sends acks to received data.
  pinMode(CAN0_INT, INPUT);  // Configuring pin for /INT input

  // ゲーム開始前の初期化
  init_board();
}

// ----------------------------------------------------------------------
void loop() {
	//
	if (!digitalRead(CAN0_INT)) {  // If CAN0_INT pin is low, read receive buffer
		CAN0.readMsgBuf(&rxId, &rxLen, rxBuf); // Read data: len = data length, buf = data byte(s)

		disp_lcd_can_message(rxId, rxLen, rxBuf);

    // ゲームロジック
    game_logic(rxId, rxLen, rxBuf);
	}

  serialInterface();
}

// ----------------------------------------------------------------------
// ゲーム開始前にボードの初期化をする
void init_board() {
  // debug message
  Serial.println("### Init board...");
  // 得点板クリア
  data[0] = SCOREBOARD_CLEAR;
  CAN0.sendMsgBuf(SCOREBOARD_RX, 0, 1, data);
  // タイマー停止
  do {
    data[0] = TIMER_STOP;
    CAN0.sendMsgBuf(TIMERBOARD_RX, 0, 1, data);
  } while (wait_for_CAN_message(TIMERBOARD_RX, TIMER_STOP) == 0);
  // ドロップターゲット上げる
  do {
    data[0] = DTARGET_UP;
    CAN0.sendMsgBuf(DTARGET0_RX, 0, 1, data);
  } while (wait_for_CAN_message(DTARGET0_RX, DTARGET_UP) == 0);
  do {
    data[0] = DTARGET_UP;
    CAN0.sendMsgBuf(DTARGET1_RX, 0, 1, data);
  } while (wait_for_CAN_message(DTARGET1_RX, DTARGET_UP) == 0);
  do {
    data[0] = DTARGET_UP;
    CAN0.sendMsgBuf(DTARGET2_RX, 0, 1, data);
  } while (wait_for_CAN_message(DTARGET2_RX, DTARGET_UP) == 0);

}

// ----------------------------------------------------------------------
// 指定した CANID とコマンドを待つ
// コマンドを送っただけでは取りこぼし等で動作しないことがあるので、
// ちゃんと返事が来るまでループするために使う
// 戻り値: 1=成功、0=失敗（まだメッセージが来ていない）
int wait_for_CAN_message(unsigned long id, byte expectedCmd) {
  if (digitalRead(CAN0_INT) == LOW) {
    CAN0.readMsgBuf(&rxId, &rxLen, rxBuf);
    if (rxId == id && rxBuf[0] == expectedCmd) {
      return 1; // success
    }
  }
  else {
    delay(1);
    return 0;
  }
}

// ----------------------------------------------------------------------
// ゲームロジック本体
void game_logic(unsigned long id, byte len, unsigned char* buf) {
  int score = 0;
  static uint8_t IsPlaying = 0; // ゲームプレイ中フラグ  0:ゲームオーバー中  1:プレイ中
  static uint8_t IsSendScore = 0; // スコア送信フラグ  0:送信不要  1:送信必要
  static uint8_t IsHitDTarget = 0; // ドロップターゲット当たりフラグ  0:当たりなし  1,2,4:当たりあり
  static unsigned long DTargetTimer[3]; // ドロップターゲット当たりから次上げるまでのタイマー
  static uint8_t IsStartTimer = 0; // ステージタイマー起動フラグ  0:停止中  1:起動中
  static unsigned long DTStageTimer = 0; // 最初のドロップターゲット当たりから始まるタイマー
  static uint8_t LastTargetState = 0, TargetState = 0;
  
  // ---------- 各アイテムの処理 ----------
  switch (id) {
    // ----- アウトホール -----
    case OUTHOLE_TX:
      switch (buf[0]) {
        case OUTHOLE_GAME_START:
          IsPlaying = 1;
          init_board();
          Serial.println("### Game started!");
          break;
        case OUTHOLE_DROP_BALL:
          IsPlaying = 0;
          Serial.println("### Game over!");
          break;
      }
      break;
    // ----- ボールセパレータ -----
    case BALLSEP_TX:
      if (buf[0] == BALLSEP_HIT) {
        score = SCORE[BALLSEP]; 
        IsSendScore = 1;
      }
      break;
    // ----- ボールスルー -----
    case BALLTHRU_TX:
      if (buf[0] == BALLTHRU_HIT) {
        score = SCORE[BALLTHRU]; 
        IsSendScore = 1;
      }
      break;
    // ----- バンパー -----
    case BUMPER0_TX: // 左バンパー
    case BUMPER1_TX: // 中バンパー
    case BUMPER2_TX: // 右バンパー
      if (buf[0] == BUMPER_HIT) {
        score = SCORE[BUMPER0]; // 左中右バンパーは同じ得点
        IsSendScore = 1;
      }
      break;
    case BUMPER3_TX: // 上バンパー
      if (buf[0] == BUMPER_HIT) {
        score = SCORE[BUMPER3]; // 上バンパーだけ別得点
        IsSendScore = 1;
      }
      break;
    // ----- スタンダップターゲット -----
    case STARGET_TX:
      if (buf[0] == STARGET_HIT) {
        score = SCORE[STARGET]; 
        IsSendScore = 1;
      }
      break;
    // ----- スリングショット -----
    case SLING0_TX: // 左スリングショット
    case SLING1_TX: // 右スリングショット
      if (buf[0] == SLING_HIT) {
        score = SCORE[SLING0]; // 左右スリングショットは同じ得点
        IsSendScore = 1;
      }
      break;
  }

  // ---------- スコア送信 ----------
  if (IsSendScore == 1) {
    data[0] = SCOREBOARD_SCORE;
    data[1] = score;
    data[2] = STAGE_MUL[STAGE];
    byte ret = CAN0.sendMsgBuf(SCOREBOARD_RX, 0, 3, data);
    if (ret == CAN_OK) {
      Serial.println("*** score send done ***");
      IsSendScore = 0; // スコア送信したらフラグを下げる
    }
    else {
      Serial.println("*** error send message ***");
    }
  }
}