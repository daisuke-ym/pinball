#ifdef DTARGET

// ----------------------------------------------------------------------
void loopDropTarget() {
  static int hue = random(360); // 色相
  static int bri = 0; // 輝度
  static uint8_t step = 5; // 輝度の変化量
  static int ss = step;
  static unsigned long led_update = 0;
  const unsigned long led_update_period = 50;
  static uint8_t state = 1; // 0: ターゲット下げ，1: ターゲット上げ
  static uint8_t lastState = 1; // 前回のターゲット状態

  if (digitalRead(SENSOR1) == HIGH) {
    delay(TARGET_DEBOUNCE);
    if (digitalRead(SENSOR1) == HIGH) {
      state = 0;
      // CAN通信でセンサ感知を通知
      CAN_DATA[0] = DTARGET_HIT;
      CAN0.sendMsgBuf(MYID + 0x100, 0, 1, CAN_DATA);
      // ターゲット下げ
      SV.write(SV_DOWN);
      delay(100);
      // LEDの色を変更
      hue = random(360);
      // デッドタイム
      delay(DEADTIME);
    }
  }

  // CANメッセージ受信処理
  if (digitalRead(CAN0_INT) == LOW) {
    CAN0.readMsgBuf(&CANID, &RX_LEN, RX_BUF);
    // コマンド DTARGET_LIFT を受け取った時の処理
    if (CANID == MYID && RX_BUF[0] == DTARGET_LIFT) {
      state = 1;
      SV.write(SV_UP);
    }
    // コマンド DTARGET_DROP を受け取った時の処理
    if (CANID == MYID && RX_BUF[0] == DTARGET_DROP) {
      state = 0;
      SV.write(SV_DOWN);
    }
  }

  // 動作テスト用
  if (digitalRead(SENSOR2) == LOW) {
    state = 0;
    SV.write(SV_DOWN);
    // LEDの色を変更
    hue = random(360);
  }
  
  // 動作テスト用
  if (digitalRead(SENSOR3) == LOW) {
    state = 1;
    SV.write(SV_UP);
  }

  // 状態が変化した場合にCANで通知
  if (state != lastState) {
    if (state == 1) {
      // ターゲットを上げた旨を送信
      CAN_DATA[0] = DTARGET_LIFT;
      CAN0.sendMsgBuf(MYID + 0x100, 0, 1, CAN_DATA);
    }
    else {
      // ターゲットを下げた旨を送信
      CAN_DATA[0] = DTARGET_DROP;
      CAN0.sendMsgBuf(MYID + 0x100, 0, 1, CAN_DATA);
    }
    lastState = state;
  }

  // LEDの明るさを変える
  if ((millis() - led_update) > led_update_period) {
    bri += ss;
    if (bri < 0) {
      ss = step;
      bri = 0;
    }
    if (bri > 100) {
      ss = -1 * step;
      bri = 100;
    }
    LEDHSB(0, hue, SAT, bri);
    led_update = millis();
  }
}

// ----------------------------------------------------------------------
void testDropTarget() {
  if (digitalRead(SENSOR1) == LOW) {
    delay(20);
    if (digitalRead(SENSOR1) == LOW) {
      LEDHSB(0, LED_YELLOW, 100, 80);
      //SV.write(SV_DOWN);
    }
  }
  if (digitalRead(SENSOR2) == LOW) {
    delay(20);
    if (digitalRead(SENSOR2) == LOW) {
      LEDHSB(0, LED_MAGENTA, 100, 80);
      //SV.write(90);
    }
  }
  if (digitalRead(SENSOR3) == LOW) {
    delay(20);
    if (digitalRead(SENSOR3) == LOW) {
      LEDHSB(0, LED_CYAN, 100, 80);
      //SV.write(SV_UP);
    }
  }
}
#endif
