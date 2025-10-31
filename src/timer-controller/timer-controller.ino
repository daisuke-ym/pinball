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

#define LEDPIN 9

MCP_CAN CAN0(3);
#define CAN0_INT 7
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
  0b10110111, // =
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

  // マイナス値はステージ番号
  if (v < 0) {
    v = -v;
    digitalWrite(LATCH, LOW);
    SPI.transfer(digits[11]); // =
    SPI.transfer(digits[v % 10]);
    digitalWrite(LATCH, HIGH);
  }
  else {
    dig[1] = int(v / 10);
    v -= dig[1] * 10;
    dig[0] = v;

    digitalWrite(LATCH, LOW);
    if (dig[1] == 0) {
      SPI.transfer(digits[12]); // blank
    }
    else {
      SPI.transfer(digits[dig[1]]);
    }
    SPI.transfer(digits[dig[0]]);
    digitalWrite(LATCH, HIGH);
  }
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
  if (CAN0.begin(MCP_STDEXT, CAN_500KBPS, MCP_16MHZ) == CAN_OK) {
    // Serial.println("MCP2515 Initialized Successfully!");
    data[0] = ~ShiftData;
  }
  else {
    // Serial.println("Error Initializing MCP2515...");
  }

  CAN0.init_Mask(0, 0, MASK0);
  CAN0.init_Mask(1, 0, MASK1);
  CAN0.init_Filt(0, 0, (CANID << 16)); // 自身のCANIDだけ受信する
  CAN0.init_Filt(1, 0, Filter1);
  CAN0.init_Filt(2, 0, Filter2); 
  CAN0.init_Filt(3, 0, Filter3);
  CAN0.init_Filt(4, 0, Filter4);
  CAN0.init_Filt(5, 0, Filter5);
  CAN0.setMode(MCP_NORMAL); // Change to normal mode to allow messages to be transmitted
  delay(random(1000));
  data[0] = CANID;
  data[1] = TIMER_TELEMETRY;
  CAN0.sendMsgBuf(MCANID, 0, 2, data);

  // 7セグの明るさ設定
  analogWrite(OE, 63);

  disp7seg(0);
}

// ----------------------------------------------------------------------
void loop() {
  static int stage_num = 0; // ステージ番号（タイマ停止中はステージ番号を表示する）
  static unsigned long stage_num_update = 0;
  const unsigned long stage_num_update_period = 1000; // ステージ番号更新間隔（ミリ秒）
  static int state = 0; // タイマの状態  0:タイマ停止  1:タイマ開始
  static unsigned long seg_update = 0;
  const unsigned long seg_update_period = 50; // ミリ秒
  static unsigned long start_time = 0;
  static unsigned long end_time = 0;
  static long tt = 0; // タイマ時間
  static unsigned long can_update = 0;
  const unsigned long can_update_period = 2000; // テレメトリー送信間隔（ミリ秒）

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
      // 受信コマンドのアンサーバック
      delay(10);
      data[0] = TIMER_COUNTDOWN;
      CAN0.sendMsgBuf(MCANID, 0, 1, data);
    }
    // TIMER_STOP コマンド（タイマ停止）を受け取った時の処理
    if (rxID == CANID && rxBuf[0] == TIMER_STOP) {
      state = 0;
      tt = 0;
      // 受信コマンドのアンサーバック
      delay(10);
      data[0] = TIMER_STOP;
      CAN0.sendMsgBuf(MCANID, 0, 1, data);
    }
    // TIMER_STAGE_NUM コマンド（ステージ番号送信）を受け取った時の処理
    if (rxID == CANID && rxBuf[0] == TIMER_STAGE_NUM) {
      stage_num = rxBuf[1];
      disp7seg(-1 * stage_num);
      // 受信コマンドのアンサーバック
      delay(10);
      data[0] = TIMER_STAGE_NUM;
      CAN0.sendMsgBuf(MCANID, 0, 1, data);
    }
  }

  // テレメトリ送信
  if ((millis() - can_update) > can_update_period) {
    can_update = millis();
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
    if ((millis() - seg_update) > seg_update_period) {
      disp7seg(tt);
      seg_update = millis();
    }
    // タイムアップ
    if (millis() > end_time) {
      // タイムアップメッセージ送信
      data[0] = TIMER_TIMEUP;
      CAN0.sendMsgBuf(MCANID, 0, 1, data);
      // タイマを止める
      state = 0;
      tt = 0;
      // 7セグ表示更新
      disp7seg(tt);
      stage_num_update = millis(); // タイムアップ後0を1秒間表示するための処理
    }
  }
  else {
    // タイマ停止中はステージ番号を表示する
    if (millis() - stage_num_update > stage_num_update_period) {
      stage_num_update = millis();
      disp7seg(-1 * stage_num);
    }
  }
}
