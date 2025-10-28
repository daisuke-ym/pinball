/*
  $Id: ballseparater-controller.ino $

  ボールセパレータ・ボールスルー制御プログラム

  Chip: ATtiny861(a)
  Clock Source: 8MHz (internal)
  Pin Mapping: New style (down each side)
  millis()/micros(): Enabled
  tinyNeoPixel port: Port B (pins 3~9, 15)
*/
#include <mcp_can.h>
#include <SPI.h>
#include <tinyNeoPixel_Static.h>

#define LEDPIN 12
#define NUMPIXELS 3
byte PIXELS[NUMPIXELS * 3];
tinyNeoPixel LED = tinyNeoPixel(NUMPIXELS, LEDPIN, NEO_GRB, PIXELS);
int HUE[NUMPIXELS]; // 各LEDの色相
int BR[NUMPIXELS];  // 各LEDの明度
int BRSS[NUMPIXELS]; // 明度の変化速度
int BRSTEP = 5; // 明度の変化度

MCP_CAN CAN0(13);
#define CAN0_INT 11
unsigned long CANID = 0; // 自分のCANID（受信用）
unsigned long MCANID = 0; // 自分のCANID（送信用）
unsigned long rxID;
unsigned char len = 0;
unsigned char rxBuf[8];
byte data[4];

const int SENSOR1 = 0;
const int SENSOR2 = 1;
const int SENSOR3 = 2;

const int SOL1 = 3;
const int SOL2 = 4;
const int NOTUSED = 4; // 未使用ピンのノイズをランダムシードに使用する

const int QH = 5;
const int SL = 6;
const int CK = 7;

const int DEBOUNCE = 10; // チャタリング無視時間(ms)
const int DEADTIME = 300; // 一度センサが検知してから，次に検知するまでの無効時間(ms)

const byte SAT = 90; // LEDの彩度

// 送信コマンド
const byte BALLSEP_TELEMETRY = 0x00; // 待機中
const byte BALLSEP_HIT       = 0x01; // ボールが当たった

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

  pinMode(LEDPIN, OUTPUT);
  for (int i = 0; i < NUMPIXELS; i++) {
    HUE[i] = random(360);
    BR[i] = 100;
    BRSS[i] = BRSTEP;
    LEDHSB(i, HUE[i], SAT, BR[i]);
  }
  LED.show();
  
  pinMode(SENSOR1, INPUT_PULLUP);
  pinMode(SENSOR2, INPUT_PULLUP);
  pinMode(SENSOR3, INPUT_PULLUP);
  //pinMode(SOL1, OUTPUT);
  //pinMode(SOL2, OUTPUT);
  randomSeed(analogRead(NOTUSED)); // 未使用ピンのノイズをランダムシードに使う
  
  pinMode(QH, INPUT_PULLUP);
  pinMode(CK, OUTPUT);
  pinMode(SL, OUTPUT);
  pinMode(CAN0_INT, INPUT);
  digitalWrite(SL, HIGH);
  digitalWrite(CK, LOW);
  delay(100);

  // DIP スイッチを読み込んで CANID を設定する
  byte ShiftData = ShiftIn(QH, CK, SL);
  CANID = CANID + ~ShiftData + 0x100 + 0x50; // 0x50 はボールセパレータ・スルー用のベースID
  MCANID = CANID + 0x100;

  // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK) {
    //Serial.println("MCP2515 Initialized Successfully!");
    data[0] = ~ShiftData;
  }
  else {
    //Serial.println("Error Initializing MCP2515...");
    for (int i = 0 ; i < 5; i++) {
      LEDHSB(0, 0, 100, 100);
      delay(100);
      LEDHSB(0, 0, 100, 0);
      delay(100);
    }
    while (1) {
      // nop
    }
  }

  CAN0.setMode(MCP_NORMAL);   // Change to normal mode to allow messages to be transmitted

  delay(random(1000));
  // 起動完了を CAN 通信で通知
  data[0] = CANID;
  data[1] = BALLSEP_TELEMETRY;
  CAN0.sendMsgBuf(MCANID, 0, 2, data);
}

// ----------------------------------------------------------------------
void loop() {
  static unsigned long led_update = 0;
  const unsigned long led_update_period = 50;
  static unsigned long can_update = 0;
  const unsigned long can_update_period = 5000; // テレメトリー送信間隔（ミリ秒）

  if (digitalRead(SENSOR1) == LOW) {
    delay(DEBOUNCE);
    if (digitalRead(SENSOR1) == LOW) {
      // CAN通信でセンサ感知を通知
      data[0] = BALLSEP_HIT;
      data[1] = 0x01;
      CAN0.sendMsgBuf(MCANID, 0, 2, data);
      // LEDの制御（乱数で色を変えて，輝度を最大にする）
      HUE[0] = random(360);
      BR[0] = 100;
      LEDHSB(0, HUE[0], SAT, BR[0]);
      while (digitalRead(SENSOR1) == LOW) {
        // wait for release SENSOR1
      }
    }
  }
  if (digitalRead(SENSOR2) == LOW) {
    delay(DEBOUNCE);
    if (digitalRead(SENSOR2) == LOW) {
      // CAN通信でセンサ感知を通知
      data[0] = BALLSEP_HIT;
      data[1] = 0x02;
      CAN0.sendMsgBuf(MCANID, 0, 2, data);
      // LEDの制御（乱数で色を変えて，輝度を最大にする）
      HUE[1] = random(360);
      BR[1] = 100;
      LEDHSB(1, HUE[1], SAT, BR[1]);
      while (digitalRead(SENSOR2) == LOW) {
        // wait for release SENSOR1
      }
    }
  }
  if (digitalRead(SENSOR3) == LOW) {
    delay(DEBOUNCE);
    if (digitalRead(SENSOR3) == LOW) {
      // CAN通信でセンサ感知を通知
      data[0] = BALLSEP_HIT;
      data[1] = 0x04;
      CAN0.sendMsgBuf(MCANID, 0, 2, data);
      // LEDの制御（乱数で色を変えて，輝度を最大にする）
      HUE[2] = random(360);
      BR[2] = 100;
      LEDHSB(2, HUE[2], SAT, BR[2]);
      while (digitalRead(SENSOR3) == LOW) {
        // wait for release SENSOR1
      }
    }
  }

  // テレメトリ送信
  if ((millis() - can_update) > can_update_period) {
    can_update = millis();
    data[0] = BALLSEP_TELEMETRY;
    data[1] = millis() / 1000 & 0xFF;
    CAN0.sendMsgBuf(MCANID, 0, 2, data);
  }

  // LEDの明るさを変える
  if ((millis() - led_update) > led_update_period) {
    for (int i = 0; i < NUMPIXELS; i++) {
      BR[i] += BRSS[i];
      if (BR[i] < 0) {
        BRSS[i]= BRSTEP;
        BR[i] = 0;
      }
      if (BR[i] > 100) {
        BRSS[i] = -1 * BRSTEP;
        BR[i] = 100;
      }
      LEDHSB(i, HUE[i], SAT, BR[i]);
    }
    led_update = millis();
  }
  LED.show();
}

// ----------------------------------------------------------------------
void loop2() {
  static unsigned long led_update = 0;
  const unsigned long led_update_period = 50;
  uint8_t SENSOR;

  if (digitalRead(SENSOR1) == LOW) {

  }

  // LEDの明るさを変える
  if ((millis() - led_update) > led_update_period) {
    for (int i = 0; i < NUMPIXELS; i++) {
      BR[i] += BRSS[i];
      if (BR[i] < 0) {
        BRSS[i]= BRSTEP;
        BR[i] = 0;
      }
      if (BR[i] > 100) {
        BRSS[i] = -1 * BRSTEP;
        BR[i] = 100;
      }
      LEDHSB(i, HUE[i], 100, BR[i]);
    }
    led_update = millis();
  }
  LED.show();
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