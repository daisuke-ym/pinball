/*
  $Id: droptarget-controller.ino 254 2023-10-18 07:47:55Z daisuke $

  ドロップターゲットの制御プログラム
  （ただし、NeoPixelとサーボを同時に使うと動作が不安定なため、NeoPixelは使わない）
  
  Chip: ATtiny861(a)
  Clock Source: 8MHz (internal)
  Pin Mapping: New style (down each side)
  tinyNeoPixel port: Port B (pins 3~9, 15)
*/
#include <mcp_can.h>
#include <SPI.h>
#include <Servo_ATTinyCore.h>
#include <tinyNeoPixel_Static.h>

#define LEDPIN 12
#define NUMPIXELS 1
byte PIXELS[NUMPIXELS * 3];
tinyNeoPixel LED = tinyNeoPixel(NUMPIXELS, LEDPIN, NEO_GRB, PIXELS);

Servo SV;
const int SV_PIN = 13;

MCP_CAN CAN0(14);
#define CAN0_INT 11
unsigned long CANID = 0; // 自分のCANID（受信用）
unsigned long MCANID = 0; // 自分のCANID（送信用）
unsigned long rxID;
unsigned char len = 0;
unsigned char rxBuf[8];
// CANのフィルタ設定
#define MASK0   0x7FF // 全ビットマスクしているので、Filter0で指定したIDのみ受信する
#define Filter0 0x030 // 受信したいCANIDを設定する
#define Filter1 0x000
#define MASK1   0x7FF
#define Filter2 0x000
#define Filter3 0x000
#define Filter4 0x000
#define Filter5 0x000

const int SENSOR1 = 0;
const int SENSOR2 = 1;
const int SENSOR3 = 2;

const int SOL1 = 3;
const int SOL2 = 4;
const int NOTUSED = 4; // 未使用ピンのノイズをランダムシードに使用する

const int QH = 5;
const int SL = 6;
const int CK = 7;

const uint8_t SV_UP = 90 + 25;
const uint8_t SV_DOWN = 90 - 31;

byte data[4];

// LED の色
const int LED_RED = 0;
const int LED_YELLOW = 60;
const int LED_GREEN = 120;
const int LED_CYAN = 180;
const int LED_BLUE = 240;
const int LED_MAGENTA = 300;

const int DEBOUNCE = 50; // チャタリング無視時間(ms)
const int DEADTIME = 300; // 一度センサが検知してから，次に検知するまでの無効時間(ms)

// 受信コマンド
const byte DTARGET_UP   = 0x10; // ターゲット上げ
const byte DTARGET_DOWN = 0x20; // ターゲット下げ

// 送信コマンド
const byte DTARGET_TELEMETRY = 0x00; // 待機中（未使用）
const byte DTARGET_HIT       = 0x01; // センサ検知

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
  SV.write(SV_UP);
  
  pinMode(SENSOR1, INPUT_PULLUP);
  pinMode(SENSOR2, INPUT_PULLUP);
  pinMode(SENSOR3, INPUT_PULLUP);
  
  pinMode(QH, INPUT_PULLUP);
  pinMode(CK, OUTPUT);
  pinMode(SL, OUTPUT);
  pinMode(CAN0_INT, INPUT);
  digitalWrite(SL, HIGH);
  digitalWrite(CK, LOW);
  randomSeed(analogRead(NOTUSED)); // 未使用ピンのノイズをランダムシードに使用する
  delay(100);

  //LEDHSB(0, random(360), 80, 100);

  // DIP スイッチを読み込んで CANID を設定する
  byte ShiftData = ShiftIn(QH, CK, SL);
  CANID = CANID + ~ShiftData + 0x100;
  MCANID = CANID + 0x100;

  // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if (CAN0.begin(MCP_STD, CAN_500KBPS, MCP_16MHZ) == CAN_OK) {
    //Serial.println("MCP2515 Initialized Successfully!");
    data[0] = ~ShiftData;
  }
  else {
    //Serial.println("Error Initializing MCP2515...");
  }

  // there are 2 mask in mcp2515, you need to set both of them
  CAN0.init_Mask(0, 0, MASK0);
  CAN0.init_Mask(1, 0, MASK1);
  CAN0.init_Filt(0, 0, CANID); // 自身のCANIDだけ受信する
  CAN0.init_Filt(1, 0, Filter1);
  CAN0.init_Filt(2, 0, Filter2); 
  CAN0.init_Filt(3, 0, Filter3);
  CAN0.init_Filt(4, 0, Filter4);
  CAN0.init_Filt(5, 0, Filter5);
  CAN0.setMode(MCP_NORMAL);   // Change to normal mode to allow messages to be transmitted
  delay(100);
  data[0] = CANID;
  data[1] = DTARGET_TELEMETRY;
  CAN0.sendMsgBuf(MCANID, 0, 2, data);

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
  /*
  static uint8_t r = random(255);
  static uint8_t g = random(255);
  static uint8_t b = random(255);
  static int br = 0; // 輝度
  static uint8_t step = 5; // 輝度の変化量
  static int ss = step;
  static unsigned long led_update = 0;
  const unsigned long led_update_period = 50;
  */
  static byte state = 1; // 0: ターゲット下げ，1: ターゲット上げ
  static unsigned long can_update = 0;
  const unsigned long can_update_period = 5000; // テレメトリー送信間隔（ミリ秒）

  if (digitalRead(SENSOR1) == LOW) {
    delay(DEBOUNCE);
    if (digitalRead(SENSOR1) == LOW) {
      state = 0;
      // CAN通信でセンサ感知を通知
      data[0] = DTARGET_HIT;
      CAN0.sendMsgBuf(MCANID, 0, 1, data);
      // ターゲット下げ
      SV.write(SV_DOWN);
      // デッドタイム
      delay(DEADTIME);
    }
  }

  if (digitalRead(CAN0_INT) == LOW) {
    // メッセージ受信
    CAN0.readMsgBuf(&rxID, &len, rxBuf);
    // コマンド DTARGET_UP を受け取った時の処理
    if (rxID == CANID && rxBuf[0] == DTARGET_UP) {
      state = 1;
      SV.write(SV_UP);
    }
    // コマンド DTARGET_DOWN を受け取った時の処理
    if (rxID == CANID && rxBuf[0] == DTARGET_DOWN) {
      state = 0;
      SV.write(SV_DOWN);
    }
  }

  if (digitalRead(SENSOR2) == LOW) {
    state = 0;
    SV.write(SV_DOWN);
  }
  
  if (digitalRead(SENSOR3) == LOW) {
    state = 1;
    SV.write(SV_UP);
  }

  // テレメトリ送信
  if ((millis() - can_update) > can_update_period) {
    can_update = millis();
    if (state == 1) {
      data[0] = DTARGET_UP;
    }
    else {
      data[0] = DTARGET_DOWN;
    }
    data[1] = millis() / 1000 & 0xFF;
    CAN0.sendMsgBuf(MCANID, 0, 2, data);
  }

  /*
  // LEDの明るさを変える
  if ((millis() - led_update) > led_update_period) {
    b += ss;
    if (b < 0) {
      ss = step;
      b = 0;
    }
    if (b > 100) {
      ss = -1 * step;
      b = 100;
    }
    LEDHSB(0, h, 100, b);
    led_update = millis();
  }
  LED.show();
  */
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
      LEDHSB(0, LED_YELLOW, 100, 80);
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