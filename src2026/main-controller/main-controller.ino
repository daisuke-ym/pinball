/*
  ピンボール台メインプログラム

  ESP32_CAN_NG ライブラリが必要
  DFPlayerMini はパチモンでなく本物を使うこと。パチモンは時々ハングアップするぞ。

*/
#include <esp32_can.h>
#include <LiquidCrystal.h>
#include <DFRobotDFPlayerMini.h>
#include "def-can.h"
#include "def-vars.h"

// ----------------------------------------------------------------------
// スイッチ関連（テスト用）
const byte SW1 = 13;
const byte SW2 = 12;
const byte SW3 = 14;
const byte SW4 = 25;

// ----------------------------------------------------------------------
// LCD関連
const byte LCD_RS = 23;
const byte LCD_EN = 22;
const byte LCD_D4 = 21;
const byte LCD_D5 = 19;
const byte LCD_D6 = 18;
const byte LCD_D7 = 5;
LiquidCrystal LCD(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

// ----------------------------------------------------------------------
// CAN関連
const gpio_num_t CAN_RX = GPIO_NUM_27;
const gpio_num_t CAN_TX = GPIO_NUM_26;
CAN_FRAME TXFRAME;

// ----------------------------------------------------------------------
// DFPlayer関連
const byte MP3B_RX = 17;
const byte MP3B_TX = 16;
const byte MP3J_RX = 33;
const byte MP3J_TX = 32;
DFRobotDFPlayerMini MP3B; // 左側（BGM担当）
DFRobotDFPlayerMini MP3J; // 右側（効果音担当）

// ----------------------------------------------------------------------
void printFrame(CAN_FRAME *message)
{
  Serial.printf("%03X", message->id);
  //Serial.print(message->id, HEX);
  if (message->extended) {
    Serial.print(" X ");
  }
  else {
    Serial.print(" S ");   
  }
  Serial.printf("(%d) ", message->length);
  //Serial.print(message->length, DEC);
  //Serial.print(") ");
  for (int i = 0; i < message->length; i++) {
    Serial.printf("%02X ", message->data.byte[i]);
    //Serial.print(message->data.byte[i], HEX);
    //Serial.print(" ");
  }
  Serial.println();
}

// ----------------------------------------------------------------------
void gotHundred(CAN_FRAME *frame)
{
  Serial.print("Got special frame!  ");
  printFrame(frame);
}

// ----------------------------------------------------------------------
void setup() {
  // スイッチ初期化
  pinMode(SW1, INPUT_PULLUP);
  pinMode(SW2, INPUT_PULLUP);
  pinMode(SW3, INPUT_PULLUP);
  pinMode(SW4, INPUT_PULLUP);

  // LCD初期化
  LCD.begin(20, 4);
  LCD.createChar(0, customChar0);
  LCD.createChar(1, customChar1);
  LCD.createChar(2, customChar2);
  LCD.createChar(3, customChar3);
  disp_lcd_header();

  // シリアル通信初期化
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, MP3B_RX, MP3B_TX); // U2左側（BGM担当）
  Serial2.begin(9600, SERIAL_8N1, MP3J_RX, MP3J_TX); // U3右側（効果音担当）

  // DFPlayer初期化
  if (MP3B.begin(Serial1)) {
    Serial.println("MP3B initialized successfully.");
    MP3B.setTimeOut(100);
    MP3B.volume(10);
    //MP3B.play(1);
  }
  else {
    Serial.println("MP3B initialization failed.");
  }
  if (MP3J.begin(Serial2)) {
    Serial.println("MP3J initialized successfully.");
    MP3J.setTimeOut(100);
    MP3J.volume(20);
    //MP3J.play(1);
  }
  else {
    Serial.println("MP3J initialization failed.");
  }
  
  // CAN初期化
  Serial.println("CAN Initializing ...");
  CAN0.setCANPins(CAN_RX, CAN_TX);
  if (CAN0.begin(CAN_BPS_500K) != 0) {
    Serial.println("Ready ...!");
  }
  else {
    Serial.println("CAN Initialization failed.");
  }

  //CAN0.watchFor(0x100, 0xF00); //setup a special filter
  CAN0.watchFor(); //then let everything else through anyway
  //CAN0.setCallback(0, gotHundred); //callback on that first special filter
}

// ----------------------------------------------------------------------
void loop() {
  byte i = 0;
  CAN_FRAME message;
  const unsigned long LcdUpdatePeriod = 500; // 500ms
  static unsigned long LcdLastUpdate = 0;

  if (CAN0.read(message)) {

    //printFrame(&message);

    disp_lcd_can_message(message.id, message.length, message.data.byte);
  }

  test_mp3();
  serial_interface();
}
