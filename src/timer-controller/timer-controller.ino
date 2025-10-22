/*
  $Id: ringled-spi7seg-controller.ino 280 2023-11-16 15:13:00Z daisuke $

  ATtiny84 によるリングLED(WS2813B)とSPI7SEGを制御するプログラム

  Chip: ATtiny84(a) (No bootloader)
  Clock Source: 8MHz (internal)
  Pin Mapping: Clockwise (recommended, like damellis core)
  tinyNeoPixel port: Port B (CW: 8~11, CCW: 0~2,11)
*/
#include <mcp_can.h>
#include <SPI.h>
#include <tinyNeoPixel_Static.h>

#define LEDPIN 9
#define NUMPIXELS 12
byte PIXELS[NUMPIXELS * 3];
tinyNeoPixel LED = tinyNeoPixel(NUMPIXELS, LEDPIN, NEO_GRB, PIXELS);

MCP_CAN CAN0(3);
#define CAN0_INT 7
unsigned long CANID = 0; // 自分のCANID（受信用）
unsigned long MCANID = 0; // 自分のCANID（送信用）
unsigned long rxID;
unsigned char len = 0;
unsigned char rxBuf[8];

byte data[4];

const byte LATCH = 2;

const byte SW1 = 10;

const byte SL = 0;
const byte CK = 1;
const byte QH = 8;

const byte OE = 0; // SL と共用
const byte CLR = 1; // CK と共用

// 7セグのフォントデータ(MSB first)
const byte digits[] = {
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
};

const byte SAT = 90; // LEDの彩度
const byte BRI = 100; // LEDの明度

// 受信コマンド
const byte TIMER_SET  = 0x10;
const byte TIMER_STOP = 0x20;
const byte TIMER_STAGE_NUM = 0x40; // ステージ番号送信

// 送信コマンド
const byte TIMER_TELEMETRY = 0x00; // テレメトリ送信
const byte TIMER_COUNTDOWN = 0x01; // カウントダウン中
const byte TIMER_TIMEUP    = 0x02; // タイムアップ

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
void disp7seg(int value) {
  int v = value;
  int dig[2];

  dig[1] = int(v / 10);
  v -= dig[1] * 10;
  dig[0] = v;

  digitalWrite(LATCH, LOW);
  if (dig[1] == 0) {
    SPI.transfer(digits[11]); // blank
  }
  else {
    SPI.transfer(digits[dig[1]]);
  }
  SPI.transfer(digits[dig[0]]);
  digitalWrite(LATCH, HIGH);
}

// ----------------------------------------------------------------------
void setup() {
  // Serial.begin(115200);

  pinMode(LEDPIN, OUTPUT);

  pinMode(SW1, INPUT_PULLUP);

  pinMode(QH, INPUT_PULLUP);
  pinMode(CK, OUTPUT);
  pinMode(SL, OUTPUT);

  pinMode(LATCH, OUTPUT);

  pinMode(CAN0_INT, INPUT);
  delay(100);

  // DIP スイッチを読み込んで CANID を設定する
  byte ShiftData = ShiftIn(QH, CK, SL);
  CANID = CANID + ~ShiftData + 0x100;
  MCANID = CANID + 0x100;

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
  data[1] = TIMER_TELEMETRY;
  CAN0.sendMsgBuf(MCANID, 0, 2, data);

  // 7セグの明るさ設定
  analogWrite(OE, 63);

  disp7seg(0);
}

// ----------------------------------------------------------------------
void loop() {
  static int state = 0; // タイマの状態  0:タイマ停止  1:タイマ開始
  static unsigned long seg_update = 0;
  const unsigned long seg_update_period = 50; // ミリ秒
  static unsigned long start_time = 0;
  static unsigned long end_time = 0;
  static long tt = 0; // タイマ時間
  static unsigned long led_update = 0;
  const unsigned long led_update_period = 74525; // マイクロ秒
  static int hue = 0; // LEDの色相
  static int blank = NUMPIXELS - 1;
  static unsigned long can_update = 0;
  const unsigned long can_update_period = 5000; // テレメトリー送信間隔（ミリ秒）

  if (!digitalRead(CAN0_INT)) {
    // メッセージ受信
    //   rxBuf[0]: コマンド
    //   rxBuf[1]: 時間
    CAN0.readMsgBuf(&rxID, &len, rxBuf);
    // echo back
    CAN0.sendMsgBuf(MCANID, 0, len, rxBuf);
    // TIMER_SET コマンド（タイマ開始）を受け取った時の処理
    if (rxID == CANID && rxBuf[0] == TIMER_SET) {
      state = 1;
      tt = rxBuf[1];
      start_time = millis();
      end_time = start_time + tt * 1000;
      data[0] = TIMER_COUNTDOWN;
      CAN0.sendMsgBuf(MCANID, 0, 1, data);
    }
    // TIMER_STOP コマンド（タイマ停止）を受け取った時の処理
    if (rxID == CANID && rxBuf[0] == TIMER_STOP) {
      state = 0;
      tt = 0;
    }
  }

  if ((millis() - can_update) > can_update_period) {
    can_update = millis();
    // テレメトリ送信
    if (state == 1) {
      data[0] = TIMER_COUNTDOWN;
    }
    else {
      data[0] = TIMER_TELEMETRY;
    }
    data[1] = millis() / 1000 & 0xFF;
    CAN0.sendMsgBuf(MCANID, 0, 2, data);
  }
  
  // タイマ動作中
  if (state == 1) {
    // カウントダウン処理
    tt = (end_time - millis()) / 1000 + 1;
    // LED表示
    if ((micros() - led_update) > led_update_period) {
      if (blank < 0) {
        blank = NUMPIXELS - 1;
      }
      for (int i = NUMPIXELS - 1; i >= 0; i--) {
        if (i == blank) {
          LEDHSB(i, (hue + 180) % 360, SAT, BRI);
        }
        else {
          LEDHSB(i, hue, SAT, BRI);
        }
      }
      blank--;
      led_update = micros();
    }
    // タイムアップ
    if (millis() > end_time) {
      // タイムアップメッセージ送信
      data[0] = TIMER_TIMEUP;
      CAN0.sendMsgBuf(MCANID, 0, 1, data);
      // タイマを止める
      state = 0;
      tt = 0;
      // 7セグ表示更新（次の点滅処理の前に0を表示させる）
      disp7seg(tt);
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

  // タイマ停止中
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
  
  // 7セグ表示更新
  if ((millis() - seg_update) > seg_update_period) {
    disp7seg(tt);
    seg_update = millis();
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