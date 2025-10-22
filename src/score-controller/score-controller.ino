/*
  $Id: score-controller.ino 280 2023-11-16 15:13:00Z daisuke $

  ATtiny84 によるSPI7SEGを制御するプログラム

  Chip: ATtiny84(a) (No bootloader)
  Clock Source: 8MHz (internal)
  Pin Mapping: Clockwise (recommended, like damellis core)
  tinyNeoPixel port: Port B (CW: 8~11, CCW: 0~2,11)
*/
#include <math.h>
#include <mcp_can.h>
#include <SPI.h>

MCP_CAN CAN0(3);
#define CAN0_INT 7
unsigned long CANID = 0; // 自分のCANID（受信用）
unsigned long MCANID = 0; // 自分のCANID（送信用）
unsigned long rxID;
unsigned char len = 0;
unsigned char rxBuf[8];

byte data[4];

const int LATCH = 2;

const int SW1 = 10;

const int SL = 0;
const int CK = 1;
const int QH = 8;

const int OE = 0; // SL と共用
const int CLR = 1; // CK と共用

// 7セグのフォントデータ
const byte digits[] = {
  // DP G F E D C B A
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

long SCORE = 0;
long TARGET = 0;
long HIGHSCORE = 0;

// 受信コマンド
const byte SCOREBOARD_SCORE = 0x10; // 得点加算
const byte SCOREBOARD_CLEAR = 0x20; // 得点クリア

// 送信コマンド
const byte SCOREBOARD_TELEMETRY = 0x00; // 待機中

// ----------------------------------------------------------------------
byte ShiftIn(int QHPin, int clockPin, int loadPin) {
  unsigned char x;
  int i;

  x = 0;
  digitalWrite(loadPin, LOW);
  digitalWrite(loadPin, HIGH);
  x = x | (digitalRead(QHPin) << 7);
  for (i = 6; i >= 0; i--) {
    digitalWrite(clockPin, HIGH);
    digitalWrite(clockPin, LOW);
    x = x | (digitalRead(QHPin) << i);
  }

  return x;
}

// ----------------------------------------------------------------------
void disp() {
  if (TARGET > SCORE + 1000000) {
    SCORE += 500000;
  }
  if (TARGET > SCORE + 100000) {
    SCORE += 50000;
  }
  if (TARGET > SCORE + 10000) {
    SCORE += 5000;
  }
  if (TARGET > SCORE + 1000) {
    SCORE += 500;
  }
  if (TARGET > SCORE + 100) {
    SCORE += 50;
  }
  if (TARGET > SCORE + 10) {
    SCORE += 5;
  }
  if (TARGET > SCORE) {
    SCORE++;
  }
  disp7seg(SCORE);
}

// ----------------------------------------------------------------------
void disp7seg(long value) {
  long v = value;
  long vh = HIGHSCORE;
  long dig[8 * 2];
  int i, j;

  //Serial.println(v);
  // ハイスコア（上段）の処理
  for (i = 15; i >= 8; i--) {
    dig[i] = int(vh / pow(10, i - 8));
    vh -= dig[i] * pow(10, i - 8);
  }
  // スコア（下段）の処理
  for (i = 7; i >= 0; i--) {
    dig[i] = int(v / pow(10, i));
    v -= dig[i] * pow(10, i);
  }
  //Serial.println();

  digitalWrite(LATCH, LOW);
  // ハイスコア（上段）の処理
  for (i = 15; i > 8; i--) {
    if (dig[i] == 0) {
      SPI.transfer(0x0);
    }
    else {
      break;
    }
  }
  for (j = i; j >= 8; j--) {
    SPI.transfer(digits[dig[j]]);
  }
  // スコア（下段）の処理
  for (i = 7; i > 0; i--) {
    if (dig[i] == 0) {
      SPI.transfer(0x0);
    }
    else {
      break;
    }
  }
  for (j = i; j >= 0; j--) {
    SPI.transfer(digits[dig[j]]);
  }
  digitalWrite(LATCH, HIGH);
}

// ----------------------------------------------------------------------
void setup() {
  // Serial.begin(115200);

  pinMode(SW1, INPUT_PULLUP);

  pinMode(QH, INPUT_PULLUP);
  pinMode(CK, OUTPUT);
  pinMode(SL, OUTPUT);

  pinMode(LATCH, OUTPUT);

  pinMode(CAN0_INT, INPUT);
  delay(100);

  // DIP スイッチを読み込んで CANID を設定する
  byte ShiftData = ShiftIn(QH, CK, SL);
  CANID = CANID + ~ShiftData + 0x100; // なぜか最後に0x100を足してやらないと目的のCANIDにならない。なぜ？
  MCANID = CANID + 0x100; // 送信用CANIDは受信用CANIDに0x100を足したもの

  // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK) {
    // Serial.println("MCP2515 Initialized Successfully!");
    data[0] = ~ShiftData;
  }
  else {
    // Serial.println("Error Initializing MCP2515...");
  }

  CAN0.setMode(MCP_NORMAL); // Change to normal mode to allow messages to be transmitted
  delay(100);
  data[0] = CANID;
  data[1] = SCOREBOARD_TELEMETRY;
  CAN0.sendMsgBuf(MCANID, 0, 2, data);

  digitalWrite(OE, LOW);
  analogWrite(OE, 40);

  /*
  demo()を入れるとマイコンのメモリ容量オーバーになってしまうのでコメントアウトしてある
  if (digitalRead(SW1) == LOW) {
    while (1) {
      demo();
    }
  }
  */
}

// ----------------------------------------------------------------------
void loop() {
  long s;
  static unsigned long seg_update = 0;
  const unsigned long seg_period = 20; // 7セグ表示更新周期（ミリ秒）

  if (!digitalRead(CAN0_INT)) {
    // メッセージ受信
    //   rxBuf[0]: コマンド
    //   rxBuf[1]: スコア
    //   rxBuf[2]: スコア倍率
    CAN0.readMsgBuf(&rxID, &len, rxBuf);
    // SCOREBOARD_SCORE コマンド（スコア受信）を受け取った時の処理
    if (rxID == CANID && rxBuf[0] == SCOREBOARD_SCORE) {
      s = rxBuf[1] * rxBuf[2];
      TARGET += s;
    }
    // SCOREBOARD_CLEAR コマンド（スコアクリア）を受け取った時の処理
    if (rxID == CANID && rxBuf[0] == SCOREBOARD_CLEAR) {
      if (SCORE > HIGHSCORE) {
        HIGHSCORE = SCORE;
      }
      SCORE = 0;
      TARGET = 0;
    }
  }

  if ((millis() - seg_update) > seg_period) {
    disp();
    seg_update = millis();
  }
}

// ----------------------------------------------------------------------
void demo() {
  static long i = 0;
  static long j;
  static unsigned long seg_update = 0;
  const unsigned long seg_period = 100;
  static unsigned long send_update = 0;
  const unsigned long send_period = 1000;
  static unsigned long score_update = 0;
  const unsigned long score_period = 2000;

  if ((millis() - score_update) > score_period) {
    TARGET += random(100000);
    if (TARGET > 99999999) {
      TARGET = 0;
      HIGHSCORE += random(1000000);
    }
    score_update = millis();
  }

  if ((millis() - seg_update) > seg_period) {
    //disp();
    seg_update = millis();
  }

  if ((millis() - send_update) > send_period) {
    j = i;
    for (int n = 8; n >= 1; n--) {
      data[8 - n] = (byte)(j / (long)pow(10, n - 1));
      j -= data[8 - n] * (long)pow(10, n - 1);
    }
    CAN0.sendMsgBuf(CANID, 0, 8, data);
    send_update = millis();
  }
}
