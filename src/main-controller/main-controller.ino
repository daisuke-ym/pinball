#include <LiquidCrystal.h>
#include <mcp_can.h>
#include <SPI.h>
#include "defines.h"

const byte LCD_RS = A0, LCD_RW = A1, LCD_E = A2, LCD_D4 = A3, LCD_D5 = A4, LCD_D6 = A5, LCD_D7 = A6;
LiquidCrystal LCD(LCD_RS, LCD_RW, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

// CAN関係
#define CAN0_INT 37
MCP_CAN CAN0(53);
unsigned long rxId;
unsigned char rxLen = 0;
unsigned char rxBuf[8];
byte data[3] = {0x00, 0x00, 0x00};

// ----------------------------------------------------------------------
void setup() {
	// Serial初期化
	Serial.begin(115200);
	// LCD初期化
	LCD.begin(20, 4);
	// CAN初期化
  if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK) {
    Serial.println("MCP2515 Initialized Successfully!");
    LCD.clear();
    LCD.setCursor(0, 0);
    LCD.print("CAN Init OK!");
    delay(1000);
  	disp_lcd_header();
  }
  else {
    Serial.println("Error Initializing MCP2515...");
    LCD.clear();
    LCD.setCursor(0, 0);
    LCD.print("CAN Error");
    while (1) {
      // nop
    }
  }
  CAN0.setMode(MCP_NORMAL);  // Set operation mode to normal so the MCP2515 sends acks to received data.
  pinMode(CAN0_INT, INPUT);  // Configuring pin for /INT input
}

// ----------------------------------------------------------------------
void loop() {
	//
	if (!digitalRead(CAN0_INT)) {  // If CAN0_INT pin is low, read receive buffer
		CAN0.readMsgBuf(&rxId, &rxLen, rxBuf); // Read data: len = data length, buf = data byte(s)

		disp_lcd_can_message(rxId, rxLen, rxBuf);
	}

  serialInterface();
}
