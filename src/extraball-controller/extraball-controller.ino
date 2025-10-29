/*
  $Id: extraball-controller.ino $

  エクストラボールの制御プログラム
  
  Chip: ATtiny861(a)
  Clock Source: 8MHz (internal)
  Pin Mapping: New style (down each side)
  tinyNeoPixel port: Port B (pins 3~9, 15)
*/
#include <mcp_can.h>
#include <SPI.h>
/*
#include <tinyNeoPixel_Static.h>

#define LEDPIN 12
#define NUMPIXELS 1
byte PIXELS[NUMPIXELS * 3];
tinyNeoPixel LED = tinyNeoPixel(NUMPIXELS, LEDPIN, NEO_GRB, PIXELS);
*/

MCP_CAN CAN0(13);
#define CAN0_INT 11
unsigned long CANID = 0; // 自分のCANID（受信用）
unsigned long MCANID = 0; // 自分のCANID（送信用）
unsigned long rxID;
unsigned char len = 0;
unsigned char rxBuf[8];
byte data[4];
// CANのフィルタ設定
#define MASK0   0x07FF0000 // 全ビットマスクしているので、Filter0で指定したIDのみ受信する
#define Filter0 0x00000000 // 受信したいCANIDを設定する
#define Filter1 0x00000000
#define MASK1   0x07FF0000
#define Filter2 0x00000000
#define Filter3 0x00000000
#define Filter4 0x00000000
#define Filter5 0x00000000

const int SENSOR1 = 0; // アウトホールセンサ
const int SENSOR2 = 1; // スタート位置センサ
const int SENSOR3 = 2; // スタートボタン
const int NOTUSED = 2; // 未使用ピンのノイズをランダムシードに使用する

const int SOL1 = 3;
const int SOL2 = 4;

const int QH = 5;
const int SL = 6;
const int CK = 7;


const int DEBOUNCE = 50; // チャタリング無視時間(ms)

// 受信コマンド
const byte EXTRABALL_SHOOT     = 0x10; // エクストラボールを打ち出せ

// 送信コマンド
const byte EXTRABALL_TELEMETRY = 0x00; // テレメトリ
const byte EXTRABALL_READY     = 0x01; // エクストラボール待機中

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
void setup() {
  //Serial.begin(115200);

  pinMode(SENSOR1, INPUT_PULLUP);
  pinMode(SENSOR2, INPUT_PULLUP);
  //pinMode(SENSOR3, INPUT_PULLUP);
  
  pinMode(SOL1, OUTPUT);
  pinMode(SOL2, OUTPUT);

  pinMode(QH, INPUT_PULLUP);
  pinMode(CK, OUTPUT);
  pinMode(SL, OUTPUT);
  pinMode(CAN0_INT, INPUT);
  digitalWrite(SL, HIGH);
  digitalWrite(CK, LOW);
  //randomSeed(analogRead(NOTUSED)); // 未使用ピンのノイズをランダムシードに使用する
  delay(100);

  //pinMode(LEDPIN, OUTPUT);
  //LEDHSB(0, random(360), 80, 100);

  // DIP スイッチを読み込んで CANID を設定する
  byte ShiftData = ShiftIn(QH, CK, SL);
  CANID = ~ShiftData + 0x100; // 何故か 0x100 を足さないと意図した CANID にならない
  MCANID = CANID + 0x100; // 送信用 CANID は 0x100 増やす

  // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if (CAN0.begin(MCP_STDEXT, CAN_500KBPS, MCP_16MHZ) == CAN_OK) {
    //Serial.println("MCP2515 Initialized Successfully!");
    data[0] = ~ShiftData;
  }
  else {
    //Serial.println("Error Initializing MCP2515...");
  }

  // CANのマスクとフィルタの設定
  CAN0.init_Mask(0, 0, MASK0);
  CAN0.init_Mask(1, 0, MASK1);
  CAN0.init_Filt(0, 0, (CANID << 16)); // 自身のCANIDだけ受信する
  CAN0.init_Filt(1, 0, Filter1);
  CAN0.init_Filt(2, 0, Filter2); 
  CAN0.init_Filt(3, 0, Filter3);
  CAN0.init_Filt(4, 0, Filter4);
  CAN0.init_Filt(5, 0, Filter5);
  CAN0.setMode(MCP_NORMAL);   // Change to normal mode to allow messages to be transmitted

  // 起動メッセージ送信
  delay(random(1000));
  if (digitalRead(SENSOR2) == LOW) {
    data[0] = EXTRABALL_READY;
  }
  else {
    data[0] = EXTRABALL_SHOOT;
  }
  CAN0.sendMsgBuf(MCANID, 0, 1, data);
}

// ----------------------------------------------------------------------
void loop() {
  static uint8_t ExtraBallShoot = 0; // 0:エクストラボール待機中    1:エクストラボール打ち出し中
  static int sensor2_state = HIGH;
  static unsigned long can_update = 0;
  const unsigned long can_update_period = 5000; // テレメトリー送信間隔（ミリ秒）

  // ボールセンサ1が検知したら次の位置までボールを打ち出す
  if (digitalRead(SENSOR1) == LOW) {
    delay(DEBOUNCE);
    if (digitalRead(SENSOR1) == LOW) {
      delay(2000);
      digitalWrite(SOL1, HIGH);
      delay(10);
      digitalWrite(SOL1, LOW);
    }
  }

  // ボールセンサ2の状態変化を検出してメッセージを送信する
  if (digitalRead(SENSOR2) != sensor2_state) {
    delay(DEBOUNCE);
    if (digitalRead(SENSOR2) != sensor2_state) {
      sensor2_state = digitalRead(SENSOR2);
      // センサ2が検知したとき
      if (sensor2_state == LOW) {
        // エクストラボール待機中のメッセージ送信
        data[0] = EXTRABALL_READY;
        CAN0.sendMsgBuf(MCANID, 0, 1, data);
        ExtraBallShoot = 0;
      }
      else {
        // エクストラボール打ち出し中のメッセージ送信
        data[0] = EXTRABALL_SHOOT;
        CAN0.sendMsgBuf(MCANID, 0, 1, data);
        ExtraBallShoot = 1;
      }
    }
  }

  // メッセージ受信
  if (digitalRead(CAN0_INT) == LOW) {
    CAN0.readMsgBuf(&rxID, &len, rxBuf);
    // EXTRABALL_SHOOT コマンドを受け取った時の処理
    if (rxID == CANID && rxBuf[0] == EXTRABALL_SHOOT) {
      // エクストラボールを打ち出す
      digitalWrite(SOL2, HIGH);
      delay(10);
      digitalWrite(SOL2, LOW);
      data[0] = EXTRABALL_SHOOT;
      CAN0.sendMsgBuf(MCANID, 0, 1, data);
      ExtraBallShoot = 1;
    }
  }

  // テレメトリー送信
  if ((millis() - can_update) > can_update_period) {
    can_update = millis();
    data[0] = EXTRABALL_TELEMETRY;
    data[1] = millis() / 1000 & 0xFF;
    CAN0.sendMsgBuf(MCANID, 0, 2, data);
  }
}