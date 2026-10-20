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
  //unsigned long starttime = micros();
  
  /*
  // デバッグ情報
  sprintf(msg, "0x%03X ", rxId);
  Serial.print(msg);
  for (int i = 0; i < len; i++) {
    sprintf(msg, "%02x ", rxBuf[i]);
    Serial.print(msg);
  }
	Serial.println();
  */
  // テレメトリー情報以外はシリアルに出力する
  if (rxBuf[0] == SEND_YOUR_NAME) {
    sprintf(msg, "0x%03X ", rxId);
    Serial.print(msg);
    for (int i = 0; i < len; i++) {
      Serial.write(rxBuf[i]);
    }
    Serial.println();
  }
  else if (rxBuf[0] != 0x00) {
    sprintf(msg, "0x%03X ", rxId);
    Serial.print(msg);
    for (int i = 0; i < len; i++) {
      sprintf(msg, "%02x ", rxBuf[i]);
      Serial.print(msg);
    }
    Serial.println();
  }

  // 各アイテムごとのステータス表示
  switch (rxId) {
    case TIMER7SEG_RX: // タイマー
      LCD.setCursor(6, 2);
      switch (rxBuf[0]) {
        case ITEM_TELEMETRY:
          LCD.setCursor(6, 3);
          LCD.write(progressChar[timerProgressIndex]);
          timerProgressIndex = (timerProgressIndex + 1) % progressLen;
          break;
        case TIMER_RUNNING:
          LCD.print("R"); // Running
          break;
        case TIMER_TIMEUP:
          LCD.print("U");
          break;
        case TIMER_STANDBY:
          LCD.print("S"); // Standby
          break;
        default:
          LCD.print("?");
          break;
      }
      break;
    case DTARGET0_RX: // 左ドロップターゲット
      LCD.setCursor(10, 2);
      switch (rxBuf[0]) {
        case ITEM_TELEMETRY:
          LCD.setCursor(10, 3);
          LCD.write(progressChar[dtarget0ProgressIndex]);
          dtarget0ProgressIndex = (dtarget0ProgressIndex + 1) % progressLen;
          break;
        case DTARGET_HIT:
          LCD.print("H");
          break;
        case DTARGET_LIFT:
          LCD.print("U");
          break;
        case DTARGET_DROP:
          LCD.print("D");
          break;
        default:
          LCD.print("?");
          break;
      }
      break;
    case DTARGET1_RX: // 中ドロップターゲット
      LCD.setCursor(9, 2);
      switch (rxBuf[0]) {
        case ITEM_TELEMETRY:
          LCD.setCursor(9, 3);
          LCD.write(progressChar[dtarget1ProgressIndex]);
          dtarget1ProgressIndex = (dtarget1ProgressIndex + 1) % progressLen;
          break;
        case DTARGET_HIT:
          LCD.print("H");
          break;
        case DTARGET_LIFT:
          LCD.print("U");
          break;
        case DTARGET_DROP:
          LCD.print("D");
          break;
        default:
          LCD.print("?");
          break;
      }
      break;
    case DTARGET2_RX: // 右ドロップターゲット
      LCD.setCursor(8, 2);
      switch (rxBuf[0]) {
        case ITEM_TELEMETRY:
          LCD.setCursor(8, 3);
          LCD.write(progressChar[dtarget2ProgressIndex]);
          dtarget2ProgressIndex = (dtarget2ProgressIndex + 1) % progressLen;
          break;
        case DTARGET_HIT:
          LCD.print("H");
          break;
        case DTARGET_LIFT:
          LCD.print("U");
          break;
        case DTARGET_DROP:
          LCD.print("D");
          break;
        default:
          LCD.print("?");
          break;
      }
      break;
    case JUMPER_RX: // ジャンパー
      LCD.setCursor(4, 2);
      switch (rxBuf[0]) {
        case ITEM_TELEMETRY:
          LCD.setCursor(4, 3);
          LCD.write(progressChar[jumperProgressIndex]);
          jumperProgressIndex = (jumperProgressIndex + 1) % progressLen;
          break;
        case JUMPER_SENSE:
          LCD.print("S");
          break;
        case JUMPER_SHOOT:
          LCD.print("H");
          break;
        case JUMPER_DROP:
          LCD.print("D");
          break;
        default:
          LCD.print("?");
          break;
      }
      break;
    case STARGET_RX: // スタンダップターゲット
      LCD.setCursor(13, 3);
      switch (rxBuf[0]) {
        case ITEM_TELEMETRY:
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
    case SLING0_RX: // 左スリングショット
      LCD.setCursor(18, 1);
      switch (rxBuf[0]) {
        case ITEM_TELEMETRY:
          LCD.write(progressChar[slingshot0ProgressIndex]);
          slingshot0ProgressIndex = (slingshot0ProgressIndex + 1) % progressLen;
          break;
        case SLING_SENSE:
          LCD.print("H");
          break;
        default:
          LCD.print("?");
          break;
      }
      break;
    case SLING1_RX: // 右スリングショット
      LCD.setCursor(17, 1);
      switch (rxBuf[0]) {
        case ITEM_TELEMETRY:
          LCD.write(progressChar[slingshot1ProgressIndex]);
          slingshot1ProgressIndex = (slingshot1ProgressIndex + 1) % progressLen;
          break;
        case SLING_SENSE:
          LCD.print("H");
          break;
        default:
          LCD.print("?");
          break;
      }
      break;
    case BUMPER0_RX: // 左バンパー
      LCD.setCursor(14, 1);
      switch (rxBuf[0]) {
        case ITEM_TELEMETRY:
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
    case BUMPER1_RX: // 中バンパー
      LCD.setCursor(13, 1);
      switch (rxBuf[0]) {
        case ITEM_TELEMETRY:
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
    case BUMPER2_RX: // 右バンパー
      LCD.setCursor(12, 1);
      switch (rxBuf[0]) {
        case ITEM_TELEMETRY:
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
    case BUMPER3_RX: // 上バンパー
      LCD.setCursor(15, 1);
      switch (rxBuf[0]) {
        case ITEM_TELEMETRY:
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
    case BALLSEP_RX: // ボールセパレータ
      LCD.setCursor(16, 3);
      switch (rxBuf[0]) {
        case ITEM_TELEMETRY:
          LCD.setCursor(15, 3);
          LCD.write(progressChar[ballsepProgressIndex]);
          ballsepProgressIndex = (ballsepProgressIndex + 1) % progressLen;
          LCD.print(" ");
          break;
        case BALLSEP_SENSE:
          LCD.print(rxBuf[1]);
          break;
        default:
          LCD.print("?");
          break;
      }
      break;
    case BALLTHRU_RX: // ボールスルー
      LCD.setCursor(19, 3);
      switch (rxBuf[0]) {
        case ITEM_TELEMETRY:
          LCD.setCursor(18, 3);
          LCD.write(progressChar[ballthruProgressIndex]);
          ballthruProgressIndex = (ballthruProgressIndex + 1) % progressLen;
          LCD.print(" ");
          break;
        case BALLTHRU_SENSE:
          LCD.print(rxBuf[1]);
          break;
        default:
          LCD.print("?");
          break;
      }
      break;
    case OUTHOLE_RX: // アウトホール
      LCD.setCursor(0, 2);
      switch (rxBuf[0]) {
        case ITEM_TELEMETRY:
          LCD.setCursor(0, 3);
          LCD.write(progressChar[outholeProgressIndex]);
          outholeProgressIndex = (outholeProgressIndex + 1) % progressLen;
          break;
        case OUTHOLE_WAIT_START:
          LCD.print("S"); // Standby (Waiting for start)
          break;
        case OUTHOLE_WAIT_BALL:
          LCD.print("N"); // Normal game
          break;
        case OUTHOLE_WAIT_EXTRABALL:
          LCD.print("B"); // Bonus game
          break;
        case OUTHOLE_DROP_BALL:
          LCD.print("D"); // Drop ball
          break;
        case OUTHOLE_DROP_EXTRABALL:
          LCD.print("E"); // drop Extra ball
          break;
        default:
          LCD.print("?");
          break;
      }
      break;
    case EXTRABALL_RX: // エクストラボール
      LCD.setCursor(2, 2);
      switch (rxBuf[0]) {
        case ITEM_TELEMETRY:
          LCD.setCursor(2, 3);
          LCD.write(progressChar[extraballProgressIndex]);
          extraballProgressIndex = (extraballProgressIndex + 1) % progressLen;
          break;
        case EXTRABALL_READY:
          LCD.print("S");
          break;
        case EXTRABALL_SHOOTED:
          LCD.print("O");
          break;
        default:
          LCD.print("?");
          break;
      }
      break;
  }

  /*
  // 処理時間計測（デバッグ情報）
  Serial.print("disp_lcd_can_message duration(us): ");
  Serial.println(micros() - starttime);
  */
}