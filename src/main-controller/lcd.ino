// ----------------------------------------------------------------------
void disp_lcd_header() {
  LCD.clear();
  LCD.setCursor(0, 0);
  LCD.print("S: ");
  
  LCD.setCursor(12, 0);
  LCD.print("BMPR SL");

  LCD.setCursor(0, 1);
  LCD.print("O E J T TGT");

  LCD.setCursor(12, 2);
  LCD.print("ST BS BT");
}

// ----------------------------------------------------------------------
void disp_lcd_can_message(unsigned long rxId, uint8_t len, uint8_t *rxBuf) {
  char msg[20];
  static uint8_t timerProgressIndex = 0;
  static uint8_t dtarget0ProgressIndex = 0;
  static uint8_t dtarget1ProgressIndex = 0;
  static uint8_t dtarget2ProgressIndex = 0;
  static uint8_t stargetProgressIndex = 0;
  static uint8_t jumperProgressIndex = 0;
  static uint8_t slingshot0ProgressIndex = 0;
  static uint8_t slingshot1ProgressIndex = 0;
  static uint8_t bumper0ProgressIndex = 0;
  static uint8_t bumper1ProgressIndex = 0;
  static uint8_t bumper2ProgressIndex = 0;
  static uint8_t bumper3ProgressIndex = 0;
  static uint8_t ballsepProgressIndex = 0;
  static uint8_t ballthruProgressIndex = 0;
  static uint8_t outholeProgressIndex = 0;
  static uint8_t extraballProgressIndex = 0;

  /*
  // Print CAN ID
  LCD.setCursor(0, 1);
  LCD.print("         "); // Clear line
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
  */
  sprintf(msg, "0x%03X ", rxId);
  Serial.print(msg);
  for (int i = 0; i < len; i++) {
    sprintf(msg, "%02x ", rxBuf[i]);
    Serial.print(msg);
  }
	Serial.println();

  // 各アイテムごとのステータス表示
  switch (rxId) {
    case TIMERBOARD_TX: // タイマー
      LCD.setCursor(6, 2);
      switch (rxBuf[0]) {
        case TIMER_TELEMETRY:
          LCD.setCursor(6, 3);
          LCD.write(progressChar[timerProgressIndex]);
          timerProgressIndex = (timerProgressIndex + 1) % progressLen;
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
      LCD.setCursor(10, 2);
      switch (rxBuf[0]) {
        case DTARGET_TELEMETRY:
          LCD.setCursor(10, 3);
          LCD.write(progressChar[dtarget0ProgressIndex]);
          dtarget0ProgressIndex = (dtarget0ProgressIndex + 1) % progressLen;
          break;
        case DTARGET_HIT:
          LCD.print("H");
          break;
        case DTARGET_UP:
          LCD.print("U");
          break;
        case DTARGET_DOWN:
          LCD.print("D");
          break;
        default:
          LCD.print("?");
          break;
      }
      break;
    case DTARGET1_TX: // 中ドロップターゲット
      LCD.setCursor(9, 2);
      switch (rxBuf[0]) {
        case DTARGET_TELEMETRY:
          LCD.setCursor(9, 3);
          LCD.write(progressChar[dtarget1ProgressIndex]);
          dtarget1ProgressIndex = (dtarget1ProgressIndex + 1) % progressLen;
          break;
        case DTARGET_HIT:
          LCD.print("H");
          break;
        case DTARGET_UP:
          LCD.print("U");
          break;
        case DTARGET_DOWN:
          LCD.print("D");
          break;
        default:
          LCD.print("?");
          break;
      }
      break;
    case DTARGET2_TX: // 右ドロップターゲット
      LCD.setCursor(8, 2);
      switch (rxBuf[0]) {
        case DTARGET_TELEMETRY:
          LCD.setCursor(8, 3);
          LCD.write(progressChar[dtarget2ProgressIndex]);
          dtarget2ProgressIndex = (dtarget2ProgressIndex + 1) % progressLen;
          break;
        case DTARGET_HIT:
          LCD.print("H");
          break;
        case DTARGET_UP:
          LCD.print("U");
          break;
        case DTARGET_DOWN:
          LCD.print("D");
          break;
        default:
          LCD.print("?");
          break;
      }
      break;
    case JUMPER_TX: // ジャンパー
      LCD.setCursor(4, 2);
      switch (rxBuf[0]) {
        case JUMPER_TELEMETRY:
          LCD.setCursor(4, 3);
          LCD.write(progressChar[jumperProgressIndex]);
          jumperProgressIndex = (jumperProgressIndex + 1) % progressLen;
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
      LCD.setCursor(13, 3);
      switch (rxBuf[0]) {
        case STARGET_TELEMETRY:
          LCD.setCursor(12, 3);
          LCD.write(progressChar[stargetProgressIndex]);
          stargetProgressIndex = (stargetProgressIndex + 1) % progressLen;
          LCD.print(" ");
          break;
        case STARGET_HIT:
          LCD.print(rxBuf[1]);
          break;
        default:
          LCD.print("?");
          break;
      }
      break;
    case SLING0_TX: // 左スリングショット
      LCD.setCursor(18, 1);
      switch (rxBuf[0]) {
        case SLING_TELEMETRY:
          LCD.write(progressChar[slingshot0ProgressIndex]);
          slingshot0ProgressIndex = (slingshot0ProgressIndex + 1) % progressLen;
          break;
        case SLING_HIT:
          LCD.print("H");
          break;
        default:
          LCD.print("?");
          break;
      }
      break;
    case SLING1_TX: // 右スリングショット
      LCD.setCursor(17, 1);
      switch (rxBuf[0]) {
        case SLING_TELEMETRY:
          LCD.write(progressChar[slingshot1ProgressIndex]);
          slingshot1ProgressIndex = (slingshot1ProgressIndex + 1) % progressLen;
          break;
        case SLING_HIT:
          LCD.print("H");
          break;
        default:
          LCD.print("?");
          break;
      }
      break;
    case BUMPER0_TX: // 左バンパー
      LCD.setCursor(14, 1);
      switch (rxBuf[0]) {
        case BUMPER_TELEMETRY:
          LCD.write(progressChar[bumper0ProgressIndex]);
          bumper0ProgressIndex = (bumper0ProgressIndex + 1) % progressLen;
          break;
        case BUMPER_HIT:
          LCD.print("H");
          break;
        default:
          LCD.print("?");
          break;
      }
      break;
    case BUMPER1_TX: // 中バンパー
      LCD.setCursor(13, 1);
      switch (rxBuf[0]) {
        case BUMPER_TELEMETRY:
          LCD.write(progressChar[bumper1ProgressIndex]);
          bumper1ProgressIndex = (bumper1ProgressIndex + 1) % progressLen;
          break;
        case BUMPER_HIT:
          LCD.print("H");
          break;
        default:
          LCD.print("?");
          break;
      }
      break;
    case BUMPER2_TX: // 右バンパー
      LCD.setCursor(12, 1);
      switch (rxBuf[0]) {
        case BUMPER_TELEMETRY:
          LCD.write(progressChar[bumper2ProgressIndex]);
          bumper2ProgressIndex = (bumper2ProgressIndex + 1) % progressLen;
          break;
        case BUMPER_HIT:
          LCD.print("H");
          break;
        default:
          LCD.print("?");
          break;
      }
      break;
    case BUMPER3_TX: // 上バンパー
      LCD.setCursor(15, 1);
      switch (rxBuf[0]) {
        case BUMPER_TELEMETRY:
          LCD.write(progressChar[bumper3ProgressIndex]);
          bumper3ProgressIndex = (bumper3ProgressIndex + 1) % progressLen;
          break;
        case BUMPER_HIT:
          LCD.print("H");
          break;
        default:
          LCD.print("?");
          break;
      }
      break;
    case BALLSEP_TX: // ボールセパレータ
      LCD.setCursor(16, 3);
      switch (rxBuf[0]) {
        case BALLSEP_TELEMETRY:
          LCD.setCursor(15, 3);
          LCD.write(progressChar[ballsepProgressIndex]);
          ballsepProgressIndex = (ballsepProgressIndex + 1) % progressLen;
          LCD.print(" ");
          break;
        case BALLSEP_HIT:
          LCD.print(rxBuf[1]);
          break;
        default:
          LCD.print("?");
          break;
      }
      break;
    case BALLTHRU_TX: // ボールスルー
      LCD.setCursor(19, 3);
      switch (rxBuf[0]) {
        case BALLTHRU_TELEMETRY:
          LCD.setCursor(18, 3);
          LCD.write(progressChar[ballthruProgressIndex]);
          ballthruProgressIndex = (ballthruProgressIndex + 1) % progressLen;
          LCD.print(" ");
          break;
        case BALLTHRU_HIT:
          LCD.print(rxBuf[1]);
          break;
        default:
          LCD.print("?");
          break;
      }
    case OUTHOLE_TX: // アウトホール
      LCD.setCursor(0, 2);
      switch (rxBuf[0]) {
        case OUTHOLE_TELEMETRY:
          LCD.setCursor(0, 3);
          LCD.write(progressChar[outholeProgressIndex]);
          outholeProgressIndex = (outholeProgressIndex + 1) % progressLen;
          break;
        case OUTHOLE_WAIT_START:
          LCD.print("W");
          break;
        case OUTHOLE_GAME_START:
          LCD.print("P");
          break;
        case OUTHOLE_DROP_BALL:
          LCD.print("D");
          break;
        case OUTHOLE_DROP_EXTRABALL:
          LCD.print("E");
          break;
        default:
          LCD.print("?");
          break;
      }
      break;
    case EXTRABALL_TX: // エクストラボール
      LCD.setCursor(2, 2);
      switch (rxBuf[0]) {
        case EXTRABALL_TELEMETRY:
          LCD.setCursor(2, 3);
          LCD.write(progressChar[extraballProgressIndex]);
          extraballProgressIndex = (extraballProgressIndex + 1) % progressLen;
          break;
        case EXTRABALL_READY:
          LCD.print("R");
          break;
        case EXTRABALL_SHOOT:
          LCD.print("S");
          break;
        default:
          LCD.print("?");
          break;
      }
      break;
  }
}