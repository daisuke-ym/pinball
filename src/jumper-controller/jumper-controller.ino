/*
  $Id: jumper-controller.ino 280 2023-11-16 15:13:00Z daisuke $

  ATtiny861 によるジャンパーと8連LED(WS2813B)を制御するプログラム

  Chip: ATtiny861(a)
  Clock Source: 8MHz (internal)
  Pin Mapping: New style (down each side)
  tinyNeoPixel port: Port B (pins 3~9, 15)
*/
#include <mcp_can.h>
#include <SPI.h>
#include <tinyNeoPixel_Static.h>

#define LEDPIN 12
#define NUMPIXELS 8
byte PIXELS[NUMPIXELS * 3];
tinyNeoPixel LED = tinyNeoPixel(NUMPIXELS, LEDPIN, NEO_GRB, PIXELS);

MCP_CAN CAN0(13);
#define CAN0_INT 11
unsigned long CANID = 0; // 自分のCANID（受信用）
unsigned long MCANID = 0; // 自分のCANID（送信用）
unsigned long rxID;
unsigned char len = 0;
unsigned char rxBuf[8];
byte data[4];

const byte SENSOR1 = 0;
const byte SENSOR2 = 1;
const byte SENSOR3 = 2;
const byte NOTUSED = 2; // 未使用ピンのノイズをランダムシードに使用する

const byte SOL1 = 3;
const byte SOL2 = 4;

const byte QH = 5;
const byte SL = 6;
const byte CK = 7;

const byte SAT = 90; // LEDの彩度
const byte BRI = 100; // LEDの明度

// 送信コマンド
const byte JUMPER_TELEMETRY  = 0x00; // 待機中
const byte JUMPER_BALL_SENSE = 0x01; // ボールを感知した
const byte JUMPER_BALL_SHOOT = 0x02; // ボールを打ち上げた
const byte JUMPER_BALL_DROP  = 0x04; // ボールが落ちた

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
  // Serial.begin(115200);

  pinMode(LEDPIN, OUTPUT);

  pinMode(SENSOR1, INPUT_PULLUP);
  pinMode(SENSOR2, INPUT_PULLUP);
  randomSeed(analogRead(NOTUSED)); // 未使用ピンのノイズをランダムシードに使用する

  pinMode(SOL1, OUTPUT);

  pinMode(QH, INPUT_PULLUP);
  pinMode(CK, OUTPUT);
  pinMode(SL, OUTPUT);
  pinMode(CAN0_INT, INPUT);
  digitalWrite(SL, HIGH);
  digitalWrite(CK, LOW);
  delay(100);

  // DIP スイッチを読み込んで CANID を設定する
  byte ShiftData = ShiftIn(QH, CK, SL);
  CANID = CANID + ~ShiftData + 0x100;
  MCANID = CANID + 0x100;

  // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK) {
    // Serial.println("MCP2515 Initialized Successfully!");
    data[0] = ~ShiftData;
    for (byte i = 0; i < 5; i++) {
      LEDHSB(0, 180, 90, 100);
      delay(100);
      LEDHSB(0, 180, 90, 0);
      delay(100);
    }
  }
  else {
    // Serial.println("Error Initializing MCP2515...");
    for (byte i = 0; i < 5; i++) {
      LEDHSB(0, 0, 90, 100);
      delay(100);
      LEDHSB(0, 0, 90, 0);
      delay(100);
    }
    while (1) {
      // nop
    }
  }

  CAN0.setMode(MCP_NORMAL); // Change to normal mode to allow messages to be transmitted
  delay(random(1000));
  data[0] = CANID;
  data[1] = JUMPER_TELEMETRY;
  CAN0.sendMsgBuf(MCANID, 0, 2, data);
}

// ----------------------------------------------------------------------
void loop() {
  static int state = 0; // ジャンパーの状態  0:ボール上がってない  1:ボール上がった
  static unsigned long led_update = 0;
  const unsigned long led_update_period = 74525; // マイクロ秒
  static int hue = 0; // LEDの色相
  static int blank = 0;

  // センサ1検知時
  if (digitalRead(SENSOR1) == LOW && state == 0) {
    delay(20);
    if (digitalRead(SENSOR1) == LOW && state == 0) {
      // ステート変更
      state = 1;
      // CAN通信でセンサ感知を通知
      data[0] = JUMPER_BALL_SENSE;
      CAN0.sendMsgBuf(MCANID, 0, 1, data);
      // 打ち出すまでのタメ
      delay(1000);
      // ジャンパー打ち出したのコマンド送信
      data[0] = JUMPER_BALL_SHOOT;
      CAN0.sendMsgBuf(MCANID, 0, 1, data);
      // ジャンパー打ち出し
      digitalWrite(SOL1, HIGH);
      delay(20);
      digitalWrite(SOL1, LOW);
      //while (digitalRead(SENSOR1) == LOW) {
        // wait until sensor is undetected
      //}
    }
  }

  // センサ2検知時
  if (digitalRead(SENSOR2) == LOW) {
    delay(2);
    if (digitalRead(SENSOR2) == LOW) {
      // ステート変更
      state = 0;
      // CAN通信でボールが落ちたことを通知
      data[0] = JUMPER_BALL_DROP;
      CAN0.sendMsgBuf(MCANID, 0, 1, data);
      // LEDを点滅させる
      for (uint8_t n = 0; n < 5; n++) {
        for (uint8_t i = 0; i < NUMPIXELS; i++) {
          LEDHSB(i, hue, SAT, BRI);
        }
        delay(50);
        for (uint8_t i = 0; i < NUMPIXELS; i++) {
          LEDHSB(i, (hue + 180) % 360, SAT, BRI);
        }
        delay(50);
      }
    }
  }

  // ボール上がった時
  if (state == 1) {
    // LED表示（現在の色相の反転色が反時計回りに回る）
    if ((micros() - led_update) > led_update_period) {
      if (blank >= NUMPIXELS) {
        blank = 0;
      }
      //for (int i = NUMPIXELS - 1; i >= 0; i--) {
      for (int i = 0; i < NUMPIXELS; i++) {
        if (i == blank) {
          LEDHSB(i, (hue + 180) % 360, SAT, BRI);
        }
        else {
          LEDHSB(i, hue, SAT, BRI);
        }
      }
      blank++;
      led_update = micros();
    }
  }

  // ボール上がってない時
  if (state == 0) {
    if ((micros() - led_update) > led_update_period) {
      hue += 1;
      if (hue >= 360) {
        hue = 0;
      }
      for (int i = 0; i < NUMPIXELS; i++) {
        LEDHSB(i, hue, SAT, BRI);
      }
      led_update = micros();
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