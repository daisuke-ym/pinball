/*
  ピンボール台メインプログラム

  ESP32_CAN_NG ライブラリが必要
  DFPlayerMini はパチモンでなく本物を使うこと。パチモンは時々ハングアップするぞ。

*/
#include <esp32_can.h>
#include <LiquidCrystal.h>
#include <DFRobotDFPlayerMini.h>
#include "def-can.h"
#include "def-vars.h"

// ----------------------------------------------------------------------
// スイッチ関連（テスト用）
const byte SW1 = 13;
const byte SW2 = 12;
const byte SW3 = 14;
const byte SW4 = 25;

// ----------------------------------------------------------------------
// LCD関連
const byte LCD_RS = 23;
const byte LCD_EN = 22;
const byte LCD_D4 = 21;
const byte LCD_D5 = 19;
const byte LCD_D6 = 18;
const byte LCD_D7 = 5;
LiquidCrystal LCD(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

// ----------------------------------------------------------------------
// CAN関連
const gpio_num_t CAN_RX = GPIO_NUM_27;
const gpio_num_t CAN_TX = GPIO_NUM_26;
CAN_FRAME TXFRAME;

// ----------------------------------------------------------------------
// DFPlayer関連
const byte MP3B_RX = 17;
const byte MP3B_TX = 16;
const byte MP3J_RX = 33;
const byte MP3J_TX = 32;
DFRobotDFPlayerMini MP3B; // 左側（BGM担当）
DFRobotDFPlayerMini MP3J; // 右側（効果音担当）

// ----------------------------------------------------------------------
void printFrame(CAN_FRAME *message)
{
  Serial.printf("%03X", message->id);
  //Serial.print(message->id, HEX);
  if (message->extended) {
    Serial.print(" X ");
  }
  else {
    Serial.print(" S ");   
  }
  Serial.printf("(%d) ", message->length);
  //Serial.print(message->length, DEC);
  //Serial.print(") ");
  for (int i = 0; i < message->length; i++) {
    Serial.printf("%02X ", message->data.byte[i]);
    //Serial.print(message->data.byte[i], HEX);
    //Serial.print(" ");
  }
  Serial.println();
}

// ----------------------------------------------------------------------
void gotHundred(CAN_FRAME *frame)
{
  Serial.print("Got special frame!  ");
  printFrame(frame);
}

// ----------------------------------------------------------------------
void setup() {
  // スイッチ初期化
  pinMode(SW1, INPUT_PULLUP);
  pinMode(SW2, INPUT_PULLUP);
  pinMode(SW3, INPUT_PULLUP);
  pinMode(SW4, INPUT_PULLUP);

  // LCD初期化
  LCD.begin(20, 4);
  LCD.createChar(0, customChar0);
  LCD.createChar(1, customChar1);
  LCD.createChar(2, customChar2);
  LCD.createChar(3, customChar3);
  disp_lcd_header();

  // シリアル通信初期化
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, MP3B_RX, MP3B_TX); // U2左側（BGM担当）
  Serial2.begin(9600, SERIAL_8N1, MP3J_RX, MP3J_TX); // U3右側（効果音担当）

  // DFPlayer初期化
  if (MP3B.begin(Serial1)) {
    Serial.println("MP3B initialized successfully.");
    MP3B.setTimeOut(100);
    MP3B.volume(5);
    //MP3B.play(1);
  }
  else {
    Serial.println("MP3B initialization failed.");
  }
  if (MP3J.begin(Serial2)) {
    Serial.println("MP3J initialized successfully.");
    MP3J.setTimeOut(100);
    MP3J.volume(10);
    //MP3J.play(1);
  }
  else {
    Serial.println("MP3J initialization failed.");
  }
  
  // CAN初期化
  Serial.println("CAN Initializing ...");
  CAN0.setCANPins(CAN_RX, CAN_TX);
  if (CAN0.begin(CAN_BPS_500K) != 0) {
    Serial.println("Ready ...!");
  }
  else {
    Serial.println("CAN Initialization failed.");
  }

  //CAN0.watchFor(0x100, 0xF00); //setup a special filter
  CAN0.watchFor(); //then let everything else through anyway
  //CAN0.setCallback(0, gotHundred); //callback on that first special filter

  initBoard();
}

// ----------------------------------------------------------------------
void loop() {
  byte i = 0;
  CAN_FRAME message;
  const unsigned long LcdUpdatePeriod = 500; // 500ms
  static unsigned long LcdLastUpdate = 0;

  if (CAN0.read(message)) {

    //printFrame(&message);

    disp_lcd_can_message(message.id, message.length, message.data.byte);
  }

  test_mp3();
  serial_interface();

  if (message.data.byte[0] != ITEM_TELEMETRY) {
    gameLogic(message.id, message.length, message.data.byte);
  }
}

// ----------------------------------------------------------------------
// ゲーム開始前にボードの初期化をする
void initBoard() {
  CAN_FRAME message;

  // debug
  Serial.println("Initializing board...");
  // BGM停止
  MP3B.stop();
  // 得点板クリア
  message.id = SCOREBOARD_TX;
  message.length = 1;
  message.data.byte[0] = SCOREBOARD_CLEAR;
  CAN0.sendFrame(message);
  delay(100);
  // ステージ番号送信
  message.id = TIMER7SEG_TX;
  message.length = 2;
  message.data.byte[0] = TIMER_STAGE_NUM;
  message.data.byte[1] = 0;
  CAN0.sendFrame(message);
  /* タイマーボードの動作が不安定なので、カウントダウン動作はしない
  // タイマー停止
  message.id = TIMER7SEG_TX;
  message.length = 1;
  message.data.byte[0] = TIMER_STOP;
  CAN0.sendFrame(message);
  */
  // 全部のドロップターゲットを上げる
  liftAllDtarget();
}
// ----------------------------------------------------------------------
// 全部のドロップターゲットを上げる（何回か出てくるので関数化してある）
void liftAllDtarget() {
  CAN_FRAME message;

  // ドロップターゲットを上げる
  message.id = DTARGET0_TX;
  message.length = 1;
  message.data.byte[0] = DTARGET_LIFT;
  CAN0.sendFrame(message);
  message.id = DTARGET1_TX;
  message.length = 1;
  message.data.byte[0] = DTARGET_LIFT;
  CAN0.sendFrame(message);
  message.id = DTARGET2_TX;
  message.length = 1;
  message.data.byte[0] = DTARGET_LIFT;
  CAN0.sendFrame(message);
}

// ----------------------------------------------------------------------
// ゲームロジック本体
void gameLogic(unsigned long id, byte len, unsigned char* buf) {
  int score = 0;
  CAN_FRAME message;
  static uint8_t isPlaying = 0; // ゲームプレイ中フラグ  0:ゲームオーバー中  1:プレイ中
  static uint8_t isBonusGame = 0; // エクストラボールの有無  0:なし（通常）  1:あり（ボーナスゲーム）
  static uint8_t isSendScore = 0; // スコア送信フラグ  0:送信不要  1:送信必要
  static uint8_t isHitDTarget = 0; // ドロップターゲット当たりフラグ  0:当たりなし  1,2,4:当たりあり
  static unsigned long DTargetTimer[3]; // ドロップターゲット当たりから次上げるまでのタイマー
  static uint8_t isStartTimer = 0; // ステージタイマー起動フラグ  0:停止中  1:起動中
  static unsigned long DTStageTimer = 0; // 最初のドロップターゲット当たりから始まるタイマー
  static uint8_t LastDTargetState = 0, DTargetState = 0;
  static unsigned long liftTargetTime = 0; // ドロップターゲットを上げるタイマー
  static uint8_t liftTargetFlag = 0;
  
  // ---------- 各アイテムの処理 ----------
  switch (id) {
    // ----- アウトホール -----
    case OUTHOLE_RX:
      switch (buf[0]) {
        case OUTHOLE_WAIT_BALL: // ゲーム開始（通常ゲーム）
          if (isPlaying == 1) {
            // エクストラボールが落ちてきた時もこのコマンドが送られてくるので何もしない
            break;
          }
          Serial.println("### Game started!");
          isPlaying = 1;
          STAGE = 0;
          initBoard();
          // BGM再生
          MP3B.loop(STAGE + 1);
          // ステージ番号送信
          message.id = TIMER7SEG_TX;
          message.length = 2;
          message.data.byte[0] = TIMER_STAGE_NUM;
          message.data.byte[1] = STAGE + 1;
          CAN0.sendFrame(message);
          break;
        case OUTHOLE_DROP_BALL:
          Serial.println("### Game over!");
          isPlaying = 0;
          STAGE = 0;
          MP3J.play(JINGLE_OUTHOLE);
          initBoard();
          break;
        case OUTHOLE_DROP_EXTRABALL:
          Serial.println("### Drop extraball (-> normal game)");
          if (isBonusGame == 1) {
            isBonusGame = 0;
            break;
          }
          break;
      }
      break;
    // ----- ボールセパレータ -----
    case BALLSEP_RX:
      if (buf[0] == BALLSEP_SENSE) {
        score = SCORE[BALLSEP]; 
        isSendScore = 1;
        MP3J.play(JINGLE_BALLSEP);
      }
      break;
    // ----- ボールスルー -----
    case BALLTHRU_RX:
      if (buf[0] == BALLTHRU_SENSE) {
        score = SCORE[BALLTHRU]; 
        isSendScore = 1;
        MP3J.play(JINGLE_BALLTHRU);
      }
      break;
    // ----- バンパー -----
    case BUMPER0_RX: // 左バンパー
      if (buf[0] == BUMPER_HIT) {
        score = SCORE[BUMPER0]; // 左中右バンパーは同じ得点
        isSendScore = 1;
        MP3J.play(JINGLE_BUMPER0);
      }
      break;
    case BUMPER1_RX: // 中バンパー
      if (buf[0] == BUMPER_HIT) {
        score = SCORE[BUMPER1]; // 左中右バンパーは同じ得点
        isSendScore = 1;
        MP3J.play(JINGLE_BUMPER1);
      }
      break;
    case BUMPER2_RX: // 右バンパー
      if (buf[0] == BUMPER_HIT) {
        score = SCORE[BUMPER2]; // 左中右バンパーは同じ得点
        isSendScore = 1;
        MP3J.play(JINGLE_BUMPER2);
      }
      break;
    case BUMPER3_RX: // 上バンパー
      if (buf[0] == BUMPER_HIT) {
        score = SCORE[BUMPER3]; // 上バンパーだけ別得点
        isSendScore = 1;
        MP3J.play(JINGLE_BUMPER3);
      }
      break;
    // ----- スタンダップターゲット -----
    case STARGET_RX:
      if (buf[0] == STARGET_HIT) {
        score = SCORE[STARGET]; 
        isSendScore = 1;
        MP3J.play(JINGLE_STARGET);
      }
      break;
    // ----- スリングショット -----
    case SLING0_RX: // 左スリングショット
    case SLING1_RX: // 右スリングショット
      if (buf[0] == SLING_SENSE) {
        score = SCORE[SLING0]; // 左右スリングショットは同じ得点
        isSendScore = 1;
        MP3J.play(JINGLE_SLING);
      }
      break;
    // ----- ジャンパー -----
    case JUMPER_RX:
      if (buf[0] == JUMPER_SENSE) {
        MP3J.play(JINGLE_JUMPER);
        Serial.println("### Jumper sense! (-> Bonus stage)");
        delay(500); // ボールがジャンパーに乗ってから打ち上げるまで少し待つ
        score = SCORE[JUMPER]; 
        isSendScore = 1;
        isBonusGame = 1; // エクストラボールが出るのでボーナスゲーム状態にする
        // エクストラボール打ち出し
        message.id = EXTRABALL_TX;
        message.length = 1;
        message.data.byte[0] = EXTRABALL_SHOOT;
        CAN0.sendFrame(message);
        message.id = OUTHOLE_TX;
        message.length = 1;
        message.data.byte[0] = OUTHOLE_WAIT_EXTRABALL;
        CAN0.sendFrame(message);
      }
      break;
    // ----- ドロップターゲット -----
    case DTARGET0_RX: // 左ドロップターゲット
      if (buf[0] == DTARGET_HIT) {
        // 当たりフラグを立てる
        DTargetState |= 0x01;
        score = SCORE[DTARGET0]; 
        isSendScore = 1;
        // ターゲットを下げる
        message.id = DTARGET0_TX;
        message.length = 1;
        message.data.byte[0] = DTARGET_DROP;
        CAN0.sendFrame(message);
        MP3J.play(JINGLE_DTARGET);
      }
      break;
    case DTARGET1_RX: // 中ドロップターゲット
      if (buf[0] == DTARGET_HIT) {
        // 当たりフラグを立てる
        DTargetState |= 0x02;
        score = SCORE[DTARGET1]; 
        isSendScore = 1;
        // ターゲットを下げる
        message.id = DTARGET1_TX;
        message.length = 1;
        message.data.byte[0] = DTARGET_DROP;
        CAN0.sendFrame(message);
        MP3J.play(JINGLE_DTARGET);
      }
      break;
    case DTARGET2_RX: // 右ドロップターゲット
      if (buf[0] == DTARGET_HIT) {
        // 当たりフラグを立てる
        DTargetState |= 0x04;
        score = SCORE[DTARGET2]; 
        isSendScore = 1;
        // ターゲットを下げる
        message.id = DTARGET2_TX;
        message.length = 1;
        message.data.byte[0] = DTARGET_DROP;
        CAN0.sendFrame(message);
        MP3J.play(JINGLE_DTARGET);
      }
      break;
    /*
    // ----- タイマー -----
    case TIMER7SEG_RX:
      switch (buf[0]) {
        case TIMER_TIMEUP:
        case TIMER_STANDBY:
          isStartTimer = 0;
          Serial.println("### Time up!");
          // ドロップターゲット上げる
          liftAllDtarget();
          DTargetState = 0;
          break;
      }
      break;
    */
  }

  /* タイマーボードの動作が不安定なので、カウントダウン動作はしない
  // ---------- タイマー開始条件 ----------
  if ((DTargetState != LastDTargetState) && (DTargetState == 0x01 || DTargetState == 0x02 || DTargetState == 0x04)) {
    isStartTimer = 1;
    // タイマー開始
    message.id = TIMER7SEG_TX;
    message.length = 2;
    message.data.byte[0] = TIMER_START;
    message.data.byte[1] = DT_TIMEOUT[STAGE];
    CAN0.sendFrame(message);
    // debug
    Serial.print("###  Start timer: ");
    Serial.print(message.data.byte[1]);
    Serial.println();
  }
  */
  LastDTargetState = DTargetState;

  // ---------- ステージ遷移 ----------
  if (DTargetState >= 7) {
    STAGE++;
    if (STAGE > NUM_STAGES) {
      STAGE = NUM_STAGES;
    }
    /*
    // タイマー停止
    isStartTimer = 0;
    message.id = TIMER7SEG_TX;
    message.length = 1;
    message.data.byte[0] = TIMER_STOP;
    CAN0.sendFrame(message);
    // debug
    Serial.print("###  Stop timer");
    Serial.println();
    */
    // BGM再生
    MP3B.loop(STAGE + 1);
    // ステージ番号送信
    message.id = TIMER7SEG_TX;
    message.length = 2;
    message.data.byte[0] = TIMER_STAGE_NUM;
    message.data.byte[1] = STAGE + 1;
    CAN0.sendFrame(message);
    // debug
    Serial.print("###  Stage: ");
    Serial.print(STAGE + 1);
    Serial.println();
    // 全部のターゲットを上げる
    liftAllDtarget();
    // フラグクリア
    DTargetState = 0;
    LastDTargetState = 0;
  }

  // ---------- スコア送信 ----------
  if (isSendScore == 1) {
    message.id = SCOREBOARD_TX;
    message.length = 3;
    message.data.byte[0] = SCOREBOARD_SCORE;
    message.data.byte[1] = score;
    message.data.byte[2] = STAGE_MUL[STAGE];
    CAN0.sendFrame(message);
    isSendScore = 0; // スコア送信したらフラグを下げる
  }

  // LCDにステージ番号を表示
  LCD.setCursor(3, 0);
  LCD.print(STAGE + 1);
}
