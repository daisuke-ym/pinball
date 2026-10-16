/*
*/
#include <mcp_can.h>
#include <SPI.h>
#include <tinyNeoPixel.h>

// ----------------------------------------------------------------------
// ピン割り当て
const byte DIPH = 4;
const byte DIPL = 5;

const byte SW1 = 1;

const byte LATCH = 2;
const byte CLR = 3;
const byte OE = 8;    // OUTPUT ENABLE (active LOW)

// ----------------------------------------------------------------------
// NeoPixel関連
const byte LEDPIN = 9;
const byte NUMPIXELS = 12;
//byte PIXELS[NUMPIXELS * 3];
tinyNeoPixel LED = tinyNeoPixel(NUMPIXELS, LEDPIN, NEO_GRB);

// ----------------------------------------------------------------------
// CAN関連
MCP_CAN CAN0(0);
const byte CAN0_INT = 13;
const byte CAN0_RST = 12;
unsigned long CANID = 0;
unsigned long rxId;
unsigned char len = 0;
unsigned char rxBuf[8];
char msgString[128];
byte data[4] = {0x00, 'T', 'M', 'R'};
// CANのフィルタ設定
unsigned long MASK0   = 0x07FF0000; // 全ビットマスクしているので、FilterXで指定したIDのみ受信する
unsigned long Filter0 = 0x00000000; // 0x000 は受信する
unsigned long Filter1 = 0x00000000; // 受信したいCANIDを設定する
unsigned long MASK1   = 0x07FF0000;
unsigned long Filter2 = 0x00000000; // 受信したいCANIDを設定する
unsigned long Filter3 = 0x00000000; // 受信したいCANIDを設定する
unsigned long Filter4 = 0x00000000; // 受信したいCANIDを設定する
unsigned long Filter5 = 0x00000000; // 受信したいCANIDを設定する
const unsigned long SEND_TELEMETRY_PERIOD = 5000; // テレメトリ送信間隔（ミリ秒）

// ----------------------------------------------------------------------
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
  0b10110111, // =
};

// ----------------------------------------------------------------------
const byte SAT = 90; // LEDの彩度
const byte BRI = 100; // LEDの明度

// ----------------------------------------------------------------------
const int VRMAP[16] = { // 20k pullup
  // NeoPixel 点灯時
  //876, 841, 807, 768, 726, 681, 635, 584,
  //530, 472, 408, 340, 266, 185, 97, 0
  // NeoPixel 消灯時
  901, 865, 829, 787, 746, 700, 653, 600,
  545, 483, 420, 349, 273, 189, 99, 0
};
const int VRMAP2[4] = {
  907, // LLLL
  834, // LLHL
  751, // LHLL
  548, // HLLL
};

// ----------------------------------------------------------------------
// CANコマンド
// 受信コマンド
const byte TIMER_START = 0x01;
const byte TIMER_STOP = 0x02;
const byte TIMER_STAGE_NUM = 0x04;

// 送信コマンド
const byte TIMER_STANDBY = 0x00;
const byte TIMER_RUNNING = 0x10;
const byte TIMER_COUNTUP = 0x20;

// 名前送信要求コマンド（全体共通 CANID = 0x000 で送信される）
const byte SEND_YOUR_NAME = 0xFF; 

// ----------------------------------------------------------------------
void disp7seg(int value) {
  int v = value;
  int dig[2];

  if (v >= 0) {
    dig[1] = int(v / 10);
    v -= dig[1] * 10;
    dig[0] = v;
  }
  else {
    // 負の値はステージ番号
    dig[1] = -1;
    v = -v;
    dig[0] = v;
  }

  digitalWrite(LATCH, LOW);
  if (dig[1] == 0) {
    SPI.transfer(digits[11]); // blank
  }
  else if (dig[1] < 0) {
    SPI.transfer(digits[12]); // =
  }
  else {
    SPI.transfer(digits[dig[1]]);
  }
  SPI.transfer(digits[dig[0]]);
  digitalWrite(LATCH, HIGH);
}

// ----------------------------------------------------------------------
void setup() {
  // 各ピンの設定
  pinMode(LEDPIN, OUTPUT);
  pinMode(SW1, INPUT_PULLUP);
  pinMode(LATCH, OUTPUT);
  pinMode(OE, OUTPUT);
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

  // 7segLEDの設定
  analogWrite(OE, 63);
  disp7seg(0);

  // DIPスイッチ確認（本番時はコメントアウト）
  //checkDipValue();
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
  const unsigned long led_update_period = 77485; // マイクロ秒 77525
  static int hue = 0; // LEDの色相
  static int blank = NUMPIXELS - 1;
  static unsigned long last_telemetry_time = 0;
  
  if (digitalRead(CAN0_INT) == LOW) {
    // メッセージ受信
    //   rxBuf[0]: コマンド 0x01:タイマ開始  0xFF:タイマ停止
    //   rxBuf[1]: 時間
    CAN0.readMsgBuf(&rxId, &len, rxBuf);
    // TIMER_START コマンド（タイマ開始）を受け取った時の処理
    if (rxId == CANID && rxBuf[0] == TIMER_START) {
      state = 1;
      tt = rxBuf[1];
      start_time = millis();
      end_time = start_time + tt * 1000;
    }
    // TIMER_STOP コマンド（タイマ停止）を受け取った時の処理
    if (rxId == CANID && rxBuf[0] == TIMER_STOP) {
      state = 0;
      tt = 0;
    }
    // TIMER_STAGE_NUM コマンドを受け取った時の処理
    if (rxId == CANID && rxBuf[0] == TIMER_STAGE_NUM) {
      tt = -1 * rxBuf[1];
    }
    // SEND_YOUR_NAME 受信処理
    if (rxId == 0x000 && rxBuf[0] == SEND_YOUR_NAME) {
      data[0] = 0xff;
      data[1] = 'T';
      data[2] = 'M';
      data[3] = 'R';
      CAN0.sendMsgBuf(CANID + 0x100, 4, data); // 送信時の CANID は自分の ID + 0x100
      // 受信したことを視認できるようにLEDを点滅させる
      for (int j = 0; j < 10; j++) {
        for (int i = 0; i < NUMPIXELS; i++) {
          LEDHSB(i, 0, 100, 100);
        }
        delay(100);
        for (int i = 0; i < NUMPIXELS; i++) {
          LEDHSB(i, 0, 0, 0);
        }
        delay(100);
      }
    }
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
      data[0] = TIMER_COUNTUP;
      CAN0.sendMsgBuf(CANID + 0x100, 0, 1, data);
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

  // テレメトリー送信
  if ((millis() - last_telemetry_time) > SEND_TELEMETRY_PERIOD) {
    // ここにテレメトリー送信処理を追加
    if (state == 0) {
      data[0] = TIMER_STANDBY;
      data[1] = millis() & 0xFF;
      CAN0.sendMsgBuf(CANID + 0x100, 0, 2, data);
    }
    else if (state == 1) {
      data[0] = TIMER_RUNNING;
      CAN0.sendMsgBuf(CANID + 0x100, 0, 1, data);
    }
    last_telemetry_time = millis();
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
// DIPスイッチを正しく読み込むための値チェック用関数
void checkDipValue() {
  unsigned long telemetry_send = 0;
  unsigned long telemetry_send_period = 1000; // ミリ秒
  int diph, dipl;
  int diphv, diplv;
  int diph_max = 0, diph_min = 1023, dipl_max = 0, dipl_min = 1023;

  while (1) {
    if ((millis() - telemetry_send) > telemetry_send_period) {
      Serial.printf("CANID: 0x%03X\n", CANID);
      int canid = 0;
      canid += mapAnalogValueToIndex(analogRead(DIPH)) << 4;
      canid += mapAnalogValueToIndex(analogRead(DIPL));
      Serial.printf("canid: 0x%03X\n", canid);
      diph = 0; dipl = 0;
      for (int i = 0; i < 16; i++) {
        diph += analogRead(DIPH);
        dipl += analogRead(DIPL);
        delay(1);
      }
      diph /= 16;
      dipl /= 16;
      diphv = mapAnalogValueToIndex(diph);
      diplv = mapAnalogValueToIndex(dipl);
      if (diph > diph_max) diph_max = diph;
      if (diph < diph_min) diph_min = diph;
      if (dipl > dipl_max) dipl_max = dipl;
      if (dipl < dipl_min) dipl_min = dipl;
      Serial.printf("DIP-H: %d (%d, %d: %d)[%d]    DIP-L: %d (%d, %d: %d)[%d]\n",
        diph, diph_min, diph_max, (diph_min + diph_max) / 2, diphv,
        dipl, dipl_min, dipl_max, (dipl_min + dipl_max) / 2, diplv);
      telemetry_send = millis();
    }

    if (digitalRead(SW1) == LOW) {
      diph_max = 0;
      diph_min = 1023;
      dipl_max = 0;
      dipl_min = 1023;
      Serial.println("reset value");
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