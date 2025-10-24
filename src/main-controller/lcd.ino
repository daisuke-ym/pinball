// ----------------------------------------------------------------------
void disp_lcd_header() {
  LCD.clear();
  LCD.setCursor(0, 0);
  LCD.print("Stage: ");
  
  LCD.setCursor(13, 0);
  LCD.print("OH EB T");

  LCD.setCursor(0, 2);
  LCD.print("BMPR STGT SL BS BT J");
}

// ----------------------------------------------------------------------
void disp_lcd_can_message(unsigned long rxId, uint8_t len, uint8_t *rxBuf) {
  char msg[13];

  // Print CAN ID
  sprintf(msg, "%03x", rxId);
  LCD.setCursor(0, 1);
  LCD.print(msg);
  Serial.print(msg);
  // Print data length
  sprintf(msg, "(%d)", len);
  LCD.print(msg);
  Serial.print(msg);
  // Print payload
  for (int i = 0; i < len; i++) {
    sprintf(msg, "%02x ", rxBuf[i]);
    LCD.print(msg);
    Serial.print(msg);
  }
	Serial.println();

  // 各アイテムごとのステータス表示
  switch (rxId) {
    case TIMERBOARD_TX: // タイマー
      LCD.setCursor(19, 1);
      switch (rxBuf[0]) {
        case TIMER_TELEMETRY:
          LCD.print("-");
          break;
        case TIMER_COUNTDOWN:
          LCD.print("C");
          break;
        case TIMER_TIMEUP:
          LCD.print("U");
          break;
        default:
          LCD.print("?");
          break;
      }
      break;
    case DTARGET0_TX: // 左ドロップターゲット
      LCD.setCursor(6, 3);
      switch (rxBuf[0]) {
        case DTARGET_TELEMETRY:
          LCD.print("-");
          break;
        case DTARGET_HIT:
          LCD.print("H");
          break;
        case DTARGET_UP:
          LCD.print("U");
          break;
        case DTARGET_DOWN:
          LCD.print("_");
          break;
        default:
          LCD.print("?");
          break;
      }
      break;
    case DTARGET1_TX: // 中ドロップターゲット
      LCD.setCursor(7, 3);
      switch (rxBuf[0]) {
        case DTARGET_TELEMETRY:
          LCD.print("-");
          break;
        case DTARGET_HIT:
          LCD.print("H");
          break;
        case DTARGET_UP:
          LCD.print("U");
          break;
        case DTARGET_DOWN:
          LCD.print("_");
          break;
        default:
          LCD.print("?");
          break;
      }
      break;
    case DTARGET2_TX: // 右ドロップターゲット
      LCD.setCursor(8, 3);
      switch (rxBuf[0]) {
        case DTARGET_TELEMETRY:
          LCD.print("-");
          break;
        case DTARGET_HIT:
          LCD.print("H");
          break;
        case DTARGET_UP:
          LCD.print("U");
          break;
        case DTARGET_DOWN:
          LCD.print("_");
          break;
        default:
          LCD.print("?");
          break;
      }
      break;
    case JUMPER_TX: // ジャンパー
      LCD.setCursor(19, 3);
      switch (rxBuf[0]) {
        case JUMPER_TELEMETRY:
          LCD.print("-");
          break;
        case JUMPER_BALL_SENSE:
          LCD.print("S");
          break;
        case JUMPER_BALL_SHOOT:
          LCD.print("H");
          break;
        case JUMPER_BALL_DROP:
          LCD.print("D");
          break;
        default:
          LCD.print("?");
          break;
      }
      break;
    case STARGET_TX: // スタンダップターゲット
      LCD.setCursor(5, 3);
      switch (rxBuf[0]) {
        case STARGET_TELEMETRY:
          LCD.print("-");
          break;
        case STARGET_HIT:
          LCD.print(rxBuf[1]);
          break;
        default:
          LCD.print("?");
          break;
      }
      break;
    default:
      // nop
      break;
  }
}