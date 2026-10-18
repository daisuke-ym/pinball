/*
  アウトホールの制御プログラム
  
  Chip: ATtiny1616
  Clock: 20MHz (internal)
*/
#include <mcp_can.h>
#include <SPI.h>
#include <Servo_megaTinyCore.h>
#include <tinyNeoPixel.h>

// ----------------------------------------------------------------------
// ピン割り当て
const byte SENSOR1 = 1; // アウトホールセンサ
const byte SENSOR2 = 2; // スタート位置センサ
const byte SENSOR3 = 3; // スタートボタン

const byte DIPH = 4;
const byte DIPL = 5;

const byte SOL1 = 10;
const byte SOL2 = 11;
//const byte NOTUSED = 10; // 未使用ピンのノイズをランダムシードに使用する

// ----------------------------------------------------------------------
// NeoPixel関連
const byte LEDPIN = 9;
const byte NUMPIXELS = 1;
tinyNeoPixel LED = tinyNeoPixel(NUMPIXELS, LEDPIN, NEO_GRB);
// LED の色
const int LED_RED = 0;
const int LED_YELLOW = 60;
const int LED_GREEN = 120;
const int LED_CYAN = 180;
const int LED_BLUE = 240;
const int LED_MAGENTA = 300;
const byte SAT = 90; // LEDの彩度
//const byte BRI = 100; // LEDの明度

// ----------------------------------------------------------------------
// サーボ関連
Servo SV;
const int SV_PIN = 8;
const uint8_t SV_CENTER = 90; // サーボの中心角度
const uint8_t SV_ANGLE = 80; // サーボを回す角度（左右対称）
const uint8_t SV_EXT = SV_CENTER + SV_ANGLE; // エクストラボール側に流す角度
const uint8_t SV_START = SV_CENTER - SV_ANGLE; // スタート側に流す角度

// ----------------------------------------------------------------------
// CAN関連
MCP_CAN CAN0(0);
const byte CAN0_INT = 13;
const byte CAN0_RST = 12;
unsigned long CANID = 0;
unsigned long rxID;
unsigned char len = 0;
unsigned char rxBuf[8];
char msgString[128];
byte data[4] = {0x00, 'O', 'H', 'L'};
// CANのフィルタ設定
unsigned long MASK0   = 0x07FF0000; // 全ビットマスクしているので、Filter0で指定したIDのみ受信する
unsigned long Filter0 = 0x00000000; // 0x000 は受信する
unsigned long Filter1 = 0x00000000; // 受信したいCANIDを設定する
unsigned long MASK1   = 0x07FF0000;
unsigned long Filter2 = 0x00000000; // 受信したいCANIDを設定する
unsigned long Filter3 = 0x00000000; // 受信したいCANIDを設定する
unsigned long Filter4 = 0x00000000; // 受信したいCANIDを設定する
unsigned long Filter5 = 0x00000000; // 受信したいCANIDを設定する
const unsigned long SEND_TELEMETRY_PERIOD = 5000; // テレメトリ送信間隔（ミリ秒）

// ----------------------------------------------------------------------
// DIPスイッチ関連
const int VRMAP[16] = { // 20k pullup
  901, 865, 829, 787, 746, 700, 653, 600,
  545, 483, 420, 349, 273, 189, 99, 0
};

// ----------------------------------------------------------------------
const int DEBOUNCE = 50; // チャタリング無視時間(ms)
const int DEADTIME = 300; // 一度センサが検知してから，次に検知するまでの無効時間(ms)

// ----------------------------------------------------------------------
// CANコマンド
// 受信コマンド
const byte OUTHOLE_WAIT_BALL      = 0x10; // ボールが落ちてくるのを待て
const byte OUTHOLE_WAIT_EXTRABALL = 0x20; // エクストラボールが落ちてくるのを待て
// 送信コマンド
const byte OUTHOLE_TELEMETRY      = 0x00; // 待機中（ボールを打ち出した後の待機状態）
const byte OUTHOLE_WAIT_START     = 0x01; // スタート待機中
const byte OUTHOLE_START_GAME     = 0x02; // ゲーム開始（スタートボタンが押された：ボールを打ち出したとみなす）
const byte OUTHOLE_DROP_BALL      = 0x04; // ボールが落ちた（ボールが落ちた瞬間だけ送信）
const byte OUTHOLE_DROP_EXTRABALL = 0x08; // エクストラボールが落ちた（ボールが落ちた瞬間だけ送信）
// 名前送信要求コマンド（全体共通 CANID = 0x000 で送信される）
const byte SEND_YOUR_NAME = 0xFF; 

// ----------------------------------------------------------------------
void setup() {
  // 各ピンの設定
  pinMode(LEDPIN, OUTPUT);
  pinMode(SENSOR1, INPUT_PULLUP);
  pinMode(SENSOR2, INPUT_PULLUP);
  pinMode(SENSOR3, INPUT_PULLUP);
  pinMode(SOL1, OUTPUT);
  // シリアル通信
  Serial.begin(115200);
  // アナログ入力の基準電圧変更
  analogReference(INTERNAL2V5);

  // DIPスイッチを読み込んでCANIDを設定（後のフィルタリングに使う）
  CANID += mapAnalogValueToIndex(analogRead(DIPH)) << 4;
  CANID += mapAnalogValueToIndex(analogRead(DIPL));
  Serial.print("MYID: 0x");
  Serial.print(CANID, HEX);
  Serial.println();
  Filter1 = CANID << 16;
  
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

  // NeoPixelの設定（全部消灯）
  LED.begin();
  for (int i = 0; i < NUMPIXELS; i++) {
    LED.setPixelColor(i, 0, 0, 0);
    //LED.setPixelColor(i, 0, 255, 0);
    LED.show();
  }

  // サーボ  
  SV.attach(SV_PIN);
  SV.write(SV_CENTER);

  /*
  while (1) {
    servo_test();
  }
  */
}

// ----------------------------------------------------------------------
void loop() {
  static uint8_t GameState = 0; // 0:初期待機中    1:ゲーム中
  static uint8_t lastGameState = 0; // 前回のゲーム状態
  static uint8_t EbState = 0;   // 0:エクストラボール待機中    1:エクストラボール打ち出し中
  static unsigned long can_update = 0;

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
        CAN0.sendMsgBuf(CANID + 0x100, 0, 1, data);
        // スタート側にボールを流す
        SV.write(SV_START);
        // ボールがスタート待機位置に到達するまで待つ
        while (digitalRead(SENSOR2) == HIGH) {
          delay(10);
        }
        SV.write(SV_CENTER);
        data[0] = OUTHOLE_WAIT_START;
        CAN0.sendMsgBuf(CANID + 0x100, 0, 1, data);
      }
      else {
        // エクストラボール打ち出し中は，エクストラボール側にボールを流す
        // メッセージ送信
        data[0] = OUTHOLE_DROP_EXTRABALL;
        CAN0.sendMsgBuf(CANID + 0x100, 0, 1, data);
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
      data[0] = OUTHOLE_START_GAME;
      CAN0.sendMsgBuf(CANID + 0x100, 0, 1, data);
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
      CAN0.sendMsgBuf(CANID + 0x100, 0, 1, data);
    }
    // OUTHOLE_WAIT_EXTRABALL コマンドを受け取った時の処理
    if (rxID == CANID && rxBuf[0] == OUTHOLE_WAIT_EXTRABALL) {
      EbState = 1;
      data[0] = OUTHOLE_WAIT_EXTRABALL;
      CAN0.sendMsgBuf(CANID + 0x100, 0, 1, data);
    }
  }

  // テレメトリー送信
  if ((millis() - can_update) > SEND_TELEMETRY_PERIOD) {
    can_update = millis();
    data[0] = OUTHOLE_TELEMETRY;
    data[1] = millis() / 1000 & 0xFF;
    CAN0.sendMsgBuf(CANID + 0x100, 0, 2, data);
    //
    if (GameState != lastGameState) {
      lastGameState = GameState;
      if (GameState == 0) {
        // 待機中に変化した時
        data[0] = OUTHOLE_WAIT_START;
        CAN0.sendMsgBuf(CANID + 0x100, 0, 1, data);
      }
      else {
        data[0] = OUTHOLE_START_GAME;
        CAN0.sendMsgBuf(CANID + 0x100 , 0, 1, data);
      }
    }
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