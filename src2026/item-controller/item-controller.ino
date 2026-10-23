/*
  アイテムの制御プログラム
  次の#defineで対象を選択してコンパイルする
  
  Chip: ATtiny1616
  Clock: 20MHz (internal)
*/

// ----------------------------------------------------------------------
// 書き込み対象のアイテムだけコメントアウトする
//#define BALLSEP
//#define BALLTHRU
//#define BUMPER
//#define JUMPER
//#define STARGET
//#define DTARGET
#define SLING
//#define FLIPPER
//#define OUTHOLE
//#define EXTRABALL
//#define TIMER7SEG
//#define LED30
//#define SCOREBOARD

// ----------------------------------------------------------------------
#include <mcp_can.h>
#include <SPI.h>
#include <Servo_megaTinyCore.h>
#include <tinyNeoPixel.h>
#include <EEPROM.h>
#include "def-can.h"

// ----------------------------------------------------------------------
// ピン割り当て
const byte SENSOR1 = 1;
const byte SENSOR2 = 2;
const byte SENSOR3 = 3;

const byte DIPH = 4;
const byte DIPL = 5;

const byte SOL1 = 10;
const byte SOL2 = 11;

// 7セグあり基板用。SENSORxと被っているがプリプロセッサで振り分けしているので問題なし
const byte LATCH = 2; 
const byte CLR = 3;
const byte OE = 8;

// ----------------------------------------------------------------------
// NeoPixel関連
const byte LEDPIN = 9;
#if defined(BUMPER) || defined(DTARGET) || defined(SLING) || defined(FLIPPER) || defined(OUTHOLE) || defined(EXTRABALL) || defined(SCOREBOARD)
  const byte NUMPIXELS = 1;
#elif defined(BALLSEP) || defined(BALLTHRU) || defined(STARGET)
  const byte NUMPIXELS = 3;
#elif defined(JUMPER)
  const byte NUMPIXELS = 8;
#elif defined(TIMER7SEG)
  const byte NUMPIXELS = 12;
#elif defined(LED30)
  const byte NUMPIXELS = 30;
#endif
tinyNeoPixel LED = tinyNeoPixel(NUMPIXELS, LEDPIN, NEO_GRB);
// LED の色
const int LED_RED = 0;
const int LED_YELLOW = 60;
const int LED_GREEN = 120;
const int LED_CYAN = 180;
const int LED_BLUE = 240;
const int LED_MAGENTA = 300;
const byte SAT = 90; // LEDの彩度
const byte BRI = 100; // LEDの明度

// ----------------------------------------------------------------------
// サーボ関連
Servo SV;
const int SV_PIN = 8;
// ドロップターゲット用
const uint8_t SV_UP   = 90 + 20; // ターゲットを上げた時の角度 25
const uint8_t SV_DOWN = 90 - 31; // ターゲットを下げた時の角度
// アウトホール用
const uint8_t SV_CENTER = 90; // サーボの中心角度
const uint8_t SV_ANGLE  = 80; // サーボを回す角度（左右対称）
const uint8_t SV_EXTRA  = SV_CENTER + SV_ANGLE; // エクストラボール側に流す角度
const uint8_t SV_START  = SV_CENTER - SV_ANGLE; // スタート側に流す角度

// ----------------------------------------------------------------------
// CAN関連
MCP_CAN CAN0(0);
const byte CAN0_INT = 13;
const byte CAN0_RST = 12;
unsigned long MYID = 0;   // 自分のID（DIPスイッチで設定）
unsigned long CANID;      // 送信したCAN ID
unsigned char RX_LEN = 0; // 受信したCANデータの長さ
unsigned char RX_BUF[8];  // 受信したCANデータのバッファ
byte CAN_DATA[4];         // 送信するCANデータのバッファ
// CANのフィルタ設定
unsigned long MASK0   = 0x07FF0000; // 全ビットマスクしているので、Filter0で指定したIDのみ受信する
unsigned long Filter0 = 0x00000000; // 0x000 は受信する
unsigned long Filter1 = 0x00000000; // 受信したいMYIDを設定する
unsigned long MASK1   = 0x07FF0000;
unsigned long Filter2 = 0x00000000; // 受信したいMYIDを設定する
unsigned long Filter3 = 0x00000000; // 受信したいMYIDを設定する
unsigned long Filter4 = 0x00000000; // 受信したいMYIDを設定する
unsigned long Filter5 = 0x00000000; // 受信したいMYIDを設定する
const unsigned long SEND_TELEMETRY_PERIOD = 2000; // テレメトリ送信間隔（ミリ秒）

// ----------------------------------------------------------------------
// DIPスイッチ関連
const int VRMAP[16] = { // 20k pullup
  901, 865, 829, 787, 746, 700, 653, 600,
  545, 483, 420, 349, 273, 189, 99, 0
};
const int VRMAP2[] = {
  901, // 0 --> 0
  787, // 3 --> 1
  273, // C --> 2
  0,   // F --> 3
};

// ----------------------------------------------------------------------
// 7セグのフォントデータ(MSB first)
const byte DIGITS[] = {
  // DP G F E D C B A
  // 負論理
  0b11000000, // 0
  0b11111001, // 1
  0b10100100, // 2
  0b10110000, // 3
  0b10011001, // 4
  0b10010010, // 5
  0b10000010, // 6
  0b11111000, // 7
  0b10000000, // 8
  0b10010000, // 9
  0b10111111, // -
  0b11111111, // blank
  0b10110111, // =
};
// 7セグのフォントデータ（得点板用）
const byte SC_DIGITS[] = {
  // DP G F E D C B A
  // 正論理
  0b00111111, // 0
  0b00000110, // 1
  0b01011011, // 2
  0b01001111, // 3
  0b01100110, // 4
  0b01101101, // 5
  0b01111101, // 6
  0b00000111, // 7
  0b01111111, // 8
  0b01101111, // 9
  0b00000000, // blank
};

// ----------------------------------------------------------------------
const int DEBOUNCE = 50; // チャタリング無視時間(ms)
const int TARGET_DEBOUNCE = 2; // フォトインタラプタを使ったセンサー用のチャタリング無視時間(ms)
const int DEADTIME = 300; // 一度センサが検知してから，次に検知するまでの無効時間(ms)

// ----------------------------------------------------------------------
// 得点板用グローバル変数
const byte MEMPTR = 0; // ハイスコアを保存するEEPROMの先頭アドレス
unsigned long SCORE = 0;
unsigned long TARGET = 0;
unsigned long HIGHSCORE = 0;

// ----------------------------------------------------------------------
void setup() {
  // シリアル通信
  Serial.begin(115200);
  // 各ピンの設定
  pinMode(LEDPIN, OUTPUT);
  #if defined(DTARGET) || defined(STARGET)
    // DTARGET、STARGETセンサーは検知でHIGHなのでプルアップしない
    pinMode(SENSOR1, INPUT);
  #else
    pinMode(SENSOR1, INPUT_PULLUP);
  #endif
  #if defined(TIMER7SEG) || defined(SCOREBOARD)
    pinMode(LATCH, OUTPUT);
    pinMode(OE, OUTPUT);
    pinMode(CLR, OUTPUT);
    digitalWrite(CLR, LOW);
    digitalWrite(CLR, HIGH);
  #elif defined(STARGET)
    // STARGETセンサーは検知でHIGHなのでプルアップしない
    pinMode(SENSOR2, INPUT);
    pinMode(SENSOR3, INPUT);
  #else
    pinMode(SENSOR2, INPUT_PULLUP);
    pinMode(SENSOR3, INPUT_PULLUP);
  #endif
  #if defined(BUMPER) || defined(JUMPER) || defined(SLING)
    pinMode(SOL1, OUTPUT);
  #endif
  #if defined(FLIPPER) || defined(OUTHOLE) || defined(EXTRABALL)
    pinMode(SOL1, OUTPUT);
    pinMode(SOL2, OUTPUT);
  #endif
  // アナログ入力の基準電圧変更
  analogReference(INTERNAL2V5);

  // NeoPixelの設定（全部消灯）
  LED.begin();
  for (int i = 0; i < NUMPIXELS; i++) {
    LED.setPixelColor(i, 0, 0, 0);
    LED.show();
  }
  delay(500);

  // DIPスイッチを読み込んでMYIDを設定（後のフィルタリングに使う）
  // DIPスイッチは2個セットで1ビットとして扱う
  // LLLLLLLL: 0    LLLLLLHH: 1    LLLLHHLL: 2    LLLLHHHH: 3
  // 1種類しかないアイテムはMYIDを固定する
  #ifdef TIMER7SEG
    MYID = TIMER7SEG_TX;
  #endif
  #ifdef OUTHOLE
    MYID = OUTHOLE_TX;
  #endif
  #ifdef EXTRABALL
    MYID = EXTRABALL_TX;
  #endif
  #ifdef DTARGET
    MYID = DTARGET0_TX;
  #endif
  #ifdef STARGET
    MYID = STARGET_TX;
  #endif
  #ifdef BUMPER
    MYID = BUMPER0_TX;
  #endif
  #ifdef SLING
    MYID = SLING0_TX;
  #endif
  #ifdef JUMPER
    MYID = JUMPER_TX;
  #endif
  #ifdef LED30
    MYID = LED30_TX;
  #endif
  #if defined(BALLSEP) || defined(BALLTHRU)
    MYID = BALLSEP_TX;
  #endif
  #ifdef SCOREBOARD
    MYID = SCOREBOARD_TX;
  #endif
  #ifdef FLIPPER
    MYID = FLIPPER0_TX;
  #endif
  #if defined(DTARGET) || defined(BUMPER) || defined(SLING) || defined(FLIPPER) || defined(BALLSEP) || defined(BALLTHRU)
    MYID += mapAnalogValueToIndex2(analogRead(DIPH)) << 2;
    MYID += mapAnalogValueToIndex2(analogRead(DIPL));
  #endif
  Serial.print("MYID: 0x");
  Serial.print(MYID, HEX);
  Serial.println();
  Filter1 = MYID << 16;
  
  // CANの設定
  pinMode(CAN0_INT, INPUT);
  pinMode(CAN0_RST, OUTPUT);
  digitalWrite(CAN0_RST, HIGH);
  if (CAN0.begin(MCP_STDEXT, CAN_500KBPS, MCP_16MHZ) == CAN_OK) {
    Serial.println("MCP2515 initialized successfully.");
  }
  else {
    Serial.println("Error initializing MCP2515.");
  }
  // CANのフィルタリング設定
  CAN0.init_Mask(0, 0, MASK0);
  CAN0.init_Filt(0, 0, Filter0);
  CAN0.init_Filt(1, 0, Filter1);
  CAN0.init_Mask(1, 0, MASK1);
  CAN0.init_Filt(2, 0, Filter2);
  CAN0.init_Filt(3, 0, Filter3);
  CAN0.init_Filt(4, 0, Filter4);
  CAN0.init_Filt(5, 0, Filter5);
  CAN0.setMode(MCP_ANY);

  // 7segLEDの設定
  #if defined(TIMER7SEG)
    analogWrite(OE, 10);
    disp7seg(0);
  #endif
  #if defined(SCOREBOARD)
    /* ATtiny1616ではbootloaderを書き込んでないとEEPROMが使えないみたい
    // EEPROMからハイスコアを読み込む
    HIGHSCORE = EEPROM.read(MEMPTR) +
               (EEPROM.read(MEMPTR + 1) << 8) +
               (EEPROM.read(MEMPTR + 2) << 16) +
               (EEPROM.read(MEMPTR + 3) << 24);
    */
    HIGHSCORE = 0;
    Serial.print("HIGHSCORE: ");
    Serial.println(HIGHSCORE);
    analogWrite(OE, 10);
    dispScore7seg(0);
  #endif

  // サーボ
  #if defined(DTARGET) || defined(OUTHOLE) 
    SV.attach(SV_PIN);
  #endif
  #ifdef DTARGET
    SV.write(SV_UP);
  #endif
  #ifdef OUTHOLE
    SV.write(SV_CENTER);
  #endif
  
  delay(100);
}

// ----------------------------------------------------------------------
void loop() {
  static unsigned long lastTelemetry = 0;

  #ifdef DTARGET
    loopDropTarget();
  #endif

  #ifdef OUTHOLE
    loopOuthole();
  #endif

  #ifdef TIMER7SEG
    loopTimer7Seg();
  #endif

  #ifdef STARGET
    loopStandupTarget();
  #endif

  #ifdef EXTRABALL
    loopExtraBall();
  #endif

  #ifdef BUMPER
    loopBumper();
  #endif

  #ifdef SLING
    loopSlingShot();
  #endif
  
  #ifdef JUMPER
    loopJumper();
  #endif

  #if defined(BALLSEP) || defined(BALLTHRU)
    loopBallSep();
  #endif

  #ifdef LED30
    loopLED30();
  #endif

  #ifdef SCOREBOARD
    loopScoreBoard();
  #endif

  #ifdef FLIPPER
    loopFlipper();
  #endif

  // テレメトリー送信
  if ((millis() - lastTelemetry) > SEND_TELEMETRY_PERIOD) {
    lastTelemetry = millis();
    CAN_DATA[0] = ITEM_TELEMETRY;
    CAN_DATA[1] = millis() / 1000 & 0xFF;
    CAN0.sendMsgBuf(MYID + 0x100, 0, 2, CAN_DATA);
  }

  // SEND_YOUR_NAME 受信処理
  // 現状この処理を入れるとCAN通信不能になるのでコメントアウトしておく
  //recvSendYourName();

  // ITEM_RESET 受信処理
  // 現状この処理を入れるとCAN通信不能になるのでコメントアウトしておく
  //recvItemReset();
}

// ----------------------------------------------------------------------
// SEND_YOUR_NAME 受信処理
void recvSendYourName() {
  if (digitalRead(CAN0_INT) == LOW) {
    CAN0.readMsgBuf(&CANID, &RX_LEN, RX_BUF);
    if (CANID == 0x000 && RX_BUF[0] == SEND_YOUR_NAME) {
      CAN_DATA[0] = 0xff;
      #ifdef DTARGET
      CAN_DATA[1] = 'D';
      CAN_DATA[2] = 'T';
      CAN_DATA[3] = 'G';
      #endif
      #ifdef TIMER7SEG
      CAN_DATA[1] = 'T';
      CAN_DATA[2] = 'M';
      CAN_DATA[3] = 'R';
      #endif
      #ifdef OUTHOLE
      CAN_DATA[1] = 'O';
      CAN_DATA[2] = 'H';
      CAN_DATA[3] = 'L';
      #endif
      #ifdef EXTRABALL
      CAN_DATA[1] = 'E';
      CAN_DATA[2] = 'X';
      CAN_DATA[3] = 'B';
      #endif
      CAN0.sendMsgBuf(MYID + 0x100, 4, CAN_DATA);
      // 受信したことを視認できるようにLEDを点滅させる
      for (int i = 0; i < 10; i++) {
        for (int j = 0; j < NUMPIXELS; j++) {
          LEDHSB(j, LED_GREEN, 100, 100);
        }
        delay(100);
        for (int j = 0; j < NUMPIXELS; j++) {
          LEDHSB(j, 0, 0, 0);
        }
        delay(100);
      }
    }
  }
}

// ----------------------------------------------------------------------
// ITEM_RESET 受信処理
void recvItemReset() {
  if (digitalRead(CAN0_INT) == LOW) {
    CAN0.readMsgBuf(&CANID, &RX_LEN, RX_BUF);
    if (CANID == 0x000 && RX_BUF[0] == ITEM_RESET) {
      // 受信したことを視認できるようにLEDを点滅させる
      for (int i = 0; i < 10; i++) {
        for (int j = 0; j < NUMPIXELS; j++) {
          LEDHSB(j, LED_RED, 100, 100);
        }
        delay(100);
        for (int j = 0; j < NUMPIXELS; j++) {
          LEDHSB(j, 0, 0, 0);
        }
        delay(100);
      }
      // ソフトウェアリセットを実行
      _PROTECTED_WRITE(RSTCTRL.SWRR, 1);
    }
  }
}

// ----------------------------------------------------------------------
// DIPスイッチから読み込んだ値を0-15に変換する（アドレス確定用）関数（中間値判定版）
unsigned long mapAnalogValueToIndex(int analogValue) {
  //Serial.print("Analog Value: ");
  //Serial.println(analogValue);

  if (analogValue >= ((VRMAP[0] + VRMAP[1]) / 2 + 1)) return 0;
  for (int i = 1; i < 15; i++) {
    if ((analogValue <= (VRMAP[i - 1] + VRMAP[i]) / 2 + 1) && (analogValue >= (VRMAP[i] + VRMAP[i + 1]) / 2)) {
      return i;
    }
  }
  return 15; // Return the last index if no match is found
}

// ----------------------------------------------------------------------
// DIPスイッチから読み込んだ値を0-15に変換する（アドレス確定用）関数（中間値判定版）
unsigned long mapAnalogValueToIndex2(int analogValue) {
  Serial.print("Analog Value: ");
  Serial.println(analogValue);

  if (analogValue >= ((VRMAP2[0] + VRMAP2[1]) / 2 + 1)) return 0;
  for (int i = 1; i < 3; i++) {
    if ((analogValue <= (VRMAP2[i - 1] + VRMAP2[i]) / 2 + 1) && (analogValue >= (VRMAP2[i] + VRMAP2[i + 1]) / 2)) {
      return i;
    }
  }
  return 3; // Return the last index if no match is found
}

// ----------------------------------------------------------------------
// HSBで表現した色でLEDを点灯させる
//   i: LEDのインデックス
//   h: 色相 (0-360)
//   s: 彩度 (0-100)
//   b: 明度 (0-100)
void LEDHSB(int i, float h, float s, float b) {
  int R, G, B;

  if (s == 0) {
    R = G = B = b * 255;
  } else {
    h /= 360.0;
    s /= 100.0;
    b /= 100.0;

    int x = (int)(h * 6);
    float f = (h * 6) - x;
    float p = b * (1 - s);
    float q = b * (1 - s * f);
    float t = b * (1 - s * (1 - f));
    
    if (x == 0) {
      R = b * 255; G = t * 255; B = p * 255;
    }
    else if (x == 1) {
      R = q * 255; G = b * 255; B = p * 255;
    }
    else if (x == 2) {
      R = p * 255; G = b * 255; B = t * 255;
    }
    else if (x == 3) {
      R = p * 255; G = q * 255; B = b * 255;
    }
    else if (x == 4) {
      R = t * 255; G = p * 255; B = b * 255;
    }
    else {
      R = b * 255; G = p * 255; B = q * 255;
    }
  }

  LED.setPixelColor(i, LED.Color(R, G, B));
  LED.show();
}