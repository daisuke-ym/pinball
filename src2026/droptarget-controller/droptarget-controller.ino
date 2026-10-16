/*
  ドロップターゲットの制御プログラム
  
  Chip: ATtiny1616
  Clock: 20MHz (internal)
*/
#include <mcp_can.h>
#include <SPI.h>
#include <Servo_megaTinyCore.h>
#include <tinyNeoPixel.h>

// ----------------------------------------------------------------------
// ピン割り当て
const byte SENSOR1 = 1;
const byte SENSOR2 = 2;
const byte SENSOR3 = 3;

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
const uint8_t SV_UP = 90 + 25;
const uint8_t SV_DOWN = 90 - 31; // init value = 31

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
byte data[4] = {0x00, 'D', 'T', 'G'};
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
const byte DTARGET_LIFT   = 0x10; // ターゲット上げ
const byte DTARGET_DROP = 0x20; // ターゲット下げ
// 送信コマンド
const byte DTARGET_TELEMETRY = 0x00; // 待機中（未使用）
const byte DTARGET_HIT       = 0x01; // センサ検知
// 名前送信要求コマンド（全体共通 CANID = 0x000 で送信される）
const byte SEND_YOUR_NAME = 0xFF; 

// ----------------------------------------------------------------------
void setup() {
  // 各ピンの設定
  pinMode(LEDPIN, OUTPUT);
  pinMode(SENSOR1, INPUT_PULLUP);
  pinMode(SENSOR2, INPUT_PULLUP);
  pinMode(SENSOR3, INPUT_PULLUP);
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
  SV.write(SV_UP);
  
  //randomSeed(analogRead(NOTUSED)); // 未使用ピンのノイズをランダムシードに使用する
  delay(100);

  LEDHSB(0, random(360), 80, 100);

  delay(random(1000));
/*
  if (digitalRead(SENSOR2) == LOW && digitalRead(SENSOR3) == LOW) {
    LEDHSB(0, 60, 100, 80);
    //SV.write(90);
    while (1) {
      //loop2();// nop
    }
  }
  */
}

// ----------------------------------------------------------------------
void loop() {
  static int hue = random(360); // 色相
  static int bri = 0; // 輝度
  static uint8_t step = 5; // 輝度の変化量
  static int ss = step;
  static unsigned long led_update = 0;
  const unsigned long led_update_period = 50;
  
  static byte state = 1; // 0: ターゲット下げ，1: ターゲット上げ
  static unsigned long can_update = 0;

  if (digitalRead(SENSOR1) == LOW) {
    delay(DEBOUNCE);
    if (digitalRead(SENSOR1) == LOW) {
      state = 0;
      // CAN通信でセンサ感知を通知
      data[0] = DTARGET_HIT;
      CAN0.sendMsgBuf(CANID + 0x100, 0, 1, data);
      // ターゲット下げ
      SV.write(SV_DOWN);
      delay(100);
      // ターゲットを下げた旨を送信
      data[0] = DTARGET_DROP;
      CAN0.sendMsgBuf(CANID + 0x100, 0, 1, data);
      // LEDの色を変更
      hue = random(360);
      // デッドタイム
      delay(DEADTIME);
    }
  }

  if (digitalRead(CAN0_INT) == LOW) {
    // メッセージ受信
    CAN0.readMsgBuf(&rxID, &len, rxBuf);
    // コマンド DTARGET_LIFT を受け取った時の処理
    if (rxID == CANID && rxBuf[0] == DTARGET_LIFT) {
      state = 1;
      SV.write(SV_UP);
      // ターゲットを上げた旨を送信
      data[0] = DTARGET_LIFT;
      CAN0.sendMsgBuf(CANID + 0x100, 0, 1, data);
    }
    // コマンド DTARGET_DROP を受け取った時の処理
    if (rxID == CANID && rxBuf[0] == DTARGET_DROP) {
      state = 0;
      SV.write(SV_DOWN);
      // ターゲットを下げた旨を送信
      data[0] = DTARGET_DROP;
      CAN0.sendMsgBuf(CANID + 0x100, 0, 1, data);
    }
    // SEND_YOUR_NAME 受信処理
    if (rxID == 0x000 && rxBuf[0] == SEND_YOUR_NAME) {
      data[0] = 0xff;
      data[1] = 'D';
      data[2] = 'T';
      data[3] = 'G';
      CAN0.sendMsgBuf(CANID + 0x100, 4, data); // 送信時の CANID は自分の ID + 0x100
      // 受信したことを視認できるようにLEDを点滅させる
      for (int j = 0; j < 10; j++) {
        LEDHSB(0, 0, 100, 100);
        delay(100);
        LEDHSB(0, 0, 0, 0);
        delay(100);
      }
    }
  }

  if (digitalRead(SENSOR2) == LOW) {
    state = 0;
    SV.write(SV_DOWN);
    // ターゲットを下げた旨を送信
    data[0] = DTARGET_DROP;
    CAN0.sendMsgBuf(CANID + 0x100, 0, 1, data);
    // LEDの色を変更
    hue = random(360);
  }
  
  if (digitalRead(SENSOR3) == LOW) {
    state = 1;
    SV.write(SV_UP);
    // ターゲットを上げた旨を送信
    data[0] = DTARGET_LIFT;
    CAN0.sendMsgBuf(CANID + 0x100, 0, 1, data);
  }

  // テレメトリ送信
  if ((millis() - can_update) > SEND_TELEMETRY_PERIOD) {
    can_update = millis();
    data[0] = DTARGET_TELEMETRY;
    data[1] = millis() / 1000 & 0xFF;
    CAN0.sendMsgBuf(CANID + 0x100, 0, 2, data);
  }

  // LEDの明るさを変える
  if ((millis() - led_update) > led_update_period) {
    bri += ss;
    if (bri < 0) {
      ss = step;
      bri = 0;
    }
    if (bri > 100) {
      ss = -1 * step;
      bri = 100;
    }
    LEDHSB(0, hue, SAT, bri);
    led_update = millis();
  }
}

// ----------------------------------------------------------------------
void loop2() {
  if (digitalRead(SENSOR1) == LOW) {
    delay(20);
    if (digitalRead(SENSOR1) == LOW) {
      LEDHSB(0, LED_YELLOW, 100, 80);
      //SV.write(SV_DOWN);
    }
  }
  if (digitalRead(SENSOR2) == LOW) {
    delay(20);
    if (digitalRead(SENSOR2) == LOW) {
      LEDHSB(0, LED_MAGENTA, 100, 80);
      //SV.write(90);
    }
  }
  if (digitalRead(SENSOR3) == LOW) {
    delay(20);
    if (digitalRead(SENSOR3) == LOW) {
      LEDHSB(0, LED_CYAN, 100, 80);
      //SV.write(SV_UP);
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