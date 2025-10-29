/*
  $Id: outhole-controller.ino $

  アウトホールの制御プログラム
  
  Chip: ATtiny861(a)
  Clock Source: 8MHz (internal)
  Pin Mapping: New style (down each side)
  tinyNeoPixel port: Port B (pins 3~9, 15)
*/
#include <mcp_can.h>
#include <SPI.h>
#include <Servo_ATTinyCore.h>
/*
#include <tinyNeoPixel.h>

#define LEDPIN 12
#define NUMPIXELS 1
tinyNeoPixel LED = tinyNeoPixel(NUMPIXELS, LEDPIN, NEO_GRB + NEO_KHZ800);
*/

Servo SV;
const int SV_PIN = 13; // パターンカットして CAN0_INT と入れ替えた

MCP_CAN CAN0(14);
#define CAN0_INT 11 // パターンカットして SV_PIN と入れ替えた
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

const int SOL1 = 3;
const int SOL2 = 4;
const int NOTUSED = 4; // 未使用ピンのノイズをランダムシードに使用する

const int QH = 5;
const int SL = 6;
const int CK = 7;

const uint8_t SV_CENTER = 90; // サーボの中心角度
const uint8_t SV_ANGLE = 80; // サーボを回す角度（左右対称）
const uint8_t SV_EXT = SV_CENTER + SV_ANGLE; // エクストラボール側に流す角度
const uint8_t SV_START = SV_CENTER - SV_ANGLE; // スタート側に流す角度


const int DEBOUNCE = 50; // チャタリング無視時間(ms)

// 受信コマンド
const byte OUTHOLE_WAIT_BALL      = 0x10; // ボールが落ちてくるのを待て
const byte OUTHOLE_WAIT_EXTRABALL = 0x20; // エクストラボールが落ちてくるのを待て

// 送信コマンド
const byte OUTHOLE_TELEMETRY      = 0x00; // 待機中（ボールを打ち出した後の待機状態）
const byte OUTHOLE_WAIT_START     = 0x01; // スタート待機中
const byte OUTHOLE_GAME_START     = 0x02; // ゲーム開始（スタートボタンが押された：ボールを打ち出したとみなす）
const byte OUTHOLE_DROP_BALL      = 0x04; // ボールが落ちた（ボールが落ちた瞬間だけ送信）
const byte OUTHOLE_DROP_EXTRABALL = 0x08; // エクストラボールが落ちた（ボールが落ちた瞬間だけ送信）

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

  SV.attach(SV_PIN);
  SV.write(90);
  
  pinMode(SENSOR1, INPUT_PULLUP);
  pinMode(SENSOR2, INPUT_PULLUP);
  pinMode(SENSOR3, INPUT_PULLUP);
  
  pinMode(SOL1, OUTPUT);

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
  MCANID = CANID + 0x100;

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

  delay(random(1000));
  data[0] = CANID;
  data[1] = OUTHOLE_TELEMETRY;
  CAN0.sendMsgBuf(MCANID, 0, 2, data);
  
  /*
  while (1) {
    servo_test();
  }
  */
}

// ----------------------------------------------------------------------
void loop() {
  static uint8_t GameState = 0; // 0:初期待機中    1:ゲーム中
  static uint8_t EbState = 0;   // 0:エクストラボール待機中    1:エクストラボール打ち出し中
  static unsigned long can_update = 0;
  const unsigned long can_update_period = 5000; // テレメトリー送信間隔（ミリ秒）

  // アウトホールの処理
  if (digitalRead(SENSOR1) == LOW) {
    delay(DEBOUNCE);
    if (digitalRead(SENSOR1) == LOW) {
      delay(1000);
      if (EbState == 0) {
        // エクストラボール待機中にボールが落ちた時はゲームオーバー
        GameState = 0;
        // メッセージ送信
        data[0] = OUTHOLE_DROP_BALL;
        CAN0.sendMsgBuf(MCANID, 0, 1, data);
        // スタート側にボールを流す
        SV.write(SV_START);
        // ボールがスタート待機位置に到達するまで待つ
        while (digitalRead(SENSOR2) == HIGH) {
          delay(10);
        }
        SV.write(SV_CENTER);
      }
      else {
        // エクストラボール打ち出し中は，エクストラボール側にボールを流す
        // メッセージ送信
        data[0] = OUTHOLE_DROP_EXTRABALL;
        CAN0.sendMsgBuf(MCANID, 0, 1, data);
        // エクストラボール側にボールを流す
        SV.write(SV_EXT);
        delay(2000);
        SV.write(SV_CENTER);
        EbState = 0;
      }
    }
  }

  // スタートボタンの処理
  if (digitalRead(SENSOR2) == LOW && digitalRead(SENSOR3) == LOW) {
    delay(DEBOUNCE);
    if (digitalRead(SENSOR2) == LOW && digitalRead(SENSOR3) == LOW) {
      GameState = 1;
      EbState = 0;
      data[0] = OUTHOLE_GAME_START;
      CAN0.sendMsgBuf(MCANID, 0, 1, data);
      digitalWrite(SOL1, HIGH);
      delay(10);
      digitalWrite(SOL1, LOW);
    }
  }

  // CANメッセージ受信処理
  if (digitalRead(CAN0_INT) == LOW) {
    // メッセージ受信
    CAN0.readMsgBuf(&rxID, &len, rxBuf);
    // OUTHOLE_WAIT_BALL コマンドを受け取った時の処理
    if (rxID == CANID && rxBuf[0] == OUTHOLE_WAIT_BALL) {
      EbState = 0;
      data[0] = OUTHOLE_WAIT_BALL;
      CAN0.sendMsgBuf(MCANID, 0, 1, data);
    }
    // OUTHOLE_WAIT_EXTRABALL コマンドを受け取った時の処理
    if (rxID == CANID && rxBuf[0] == OUTHOLE_WAIT_EXTRABALL) {
      EbState = 1;
      data[0] = OUTHOLE_WAIT_EXTRABALL;
      CAN0.sendMsgBuf(MCANID, 0, 1, data);
    }
  }

  // テレメトリー送信
  if ((millis() - can_update) > can_update_period) {
    can_update = millis();
    data[0] = OUTHOLE_TELEMETRY;
    data[1] = millis() / 1000 & 0xFF;
    CAN0.sendMsgBuf(MCANID, 0, 2, data);
  }
}

// ----------------------------------------------------------------------
void servo_test() {
  if (digitalRead(SENSOR1) == LOW) {
    delay(20);
    if (digitalRead(SENSOR1) == LOW) {
      SV.write(SV_EXT);
    }
  }
  if (digitalRead(SENSOR2) == LOW) {
    delay(20);
    if (digitalRead(SENSOR2) == LOW) {
      SV.write(90);
    }
  }
  if (digitalRead(SENSOR3) == LOW) {
    delay(20);
    if (digitalRead(SENSOR3) == LOW) {
      SV.write(SV_START);
    }
  }
}