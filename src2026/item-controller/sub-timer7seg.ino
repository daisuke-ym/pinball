#ifdef TIMER7SEG

// ----------------------------------------------------------------------
void loopTimer7Seg() {
  static int state = 0; // タイマの状態  0:タイマ停止  1:タイマ開始
  static int lastState = 2;
  static unsigned long seg_update = 0;
  const unsigned long seg_update_period = 50; // ミリ秒
  static unsigned long start_time = 0;
  static unsigned long end_time = 0;
  static long tt = 0; // タイマ時間
  static unsigned long led_update = 0;
  const unsigned long led_update_period = 77485; // マイクロ秒 77525
  static int hue = 0; // LEDの色相
  static int blank = NUMPIXELS - 1;
  
  if (digitalRead(CAN0_INT) == LOW) {
    // メッセージ受信
    //   RX_BUF[0]: コマンド
    //   RX_BUF[1]: 時間
    CAN0.readMsgBuf(&CANID, &RX_LEN, RX_BUF);
    // TIMER_START コマンド（タイマ開始）を受け取った時の処理
    if (CANID == MYID && RX_BUF[0] == TIMER_START) {
      state = 1;
      tt = RX_BUF[1];
      start_time = millis();
      end_time = start_time + tt * 1000;
    }
    // TIMER_STOP コマンド（タイマ停止）を受け取った時の処理
    if (CANID == MYID && RX_BUF[0] == TIMER_STOP) {
      state = 0;
      tt = 0;
    }
    // TIMER_STAGE_NUM コマンドを受け取った時の処理
    if (CANID == MYID && RX_BUF[0] == TIMER_STAGE_NUM) {
      tt = -1 * RX_BUF[1];
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
      CAN_DATA[0] = TIMER_TIMEUP;
      CAN0.sendMsgBuf(MYID + 0x100, 0, 1, CAN_DATA);
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

  // 状態が変化した場合にCANで通知
  if (state != lastState) {
    lastState = state;
    Serial.print("State changed to: ");
    Serial.println(state);
    // 必要に応じて状態変化時の処理をここに追加
    switch (state) {
      case 0:
        // タイマ停止中の処理
        CAN_DATA[0] = TIMER_STANDBY;
        break;
      case 1:
        // タイマ動作中の処理
        CAN_DATA[0] = TIMER_RUNNING;
        break;
    }
    CAN0.sendMsgBuf(MYID + 0x100, 0, 1, CAN_DATA);
  }
}

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
    SPI.transfer(DIGITS[11]); // blank
  }
  else if (dig[1] < 0) {
    SPI.transfer(DIGITS[12]); // =
  }
  else {
    SPI.transfer(DIGITS[dig[1]]);
  }
  SPI.transfer(DIGITS[dig[0]]);
  digitalWrite(LATCH, HIGH);
}
#endif
