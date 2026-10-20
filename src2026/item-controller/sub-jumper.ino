#ifdef JUMPER

// ----------------------------------------------------------------------
void loopJumper() {
  static int state = 0; // ジャンパーの状態  0:ボール上がってない  1:ボール上がった
  static int hue = 0; // LEDの色相
  static int blank = 0;
  static unsigned long led_update = 0;
  const unsigned long led_update_period = 74525; // マイクロ秒
  
  // センサ1検知時
  if (digitalRead(SENSOR1) == LOW && state == 0) {
    delay(20);
    if (digitalRead(SENSOR1) == LOW && state == 0) {
      // ステート変更
      state = 1;
      // CAN通信でセンサ感知を通知
      CAN_DATA[0] = JUMPER_SENSE;
      CAN0.sendMsgBuf(MYID + 0x100, 0, 1, CAN_DATA);
      // 打ち出すまでのタメ
      delay(1000);
      // ジャンパー打ち出したのコマンド送信
      CAN_DATA[0] = JUMPER_SHOOT;
      CAN0.sendMsgBuf(MYID + 0x100, 0, 1, CAN_DATA);
      // ジャンパー打ち出し
      digitalWrite(SOL1, HIGH);
      delay(20);
      digitalWrite(SOL1, LOW);
      //while (digitalRead(SENSOR1) == LOW) {
        // wait until sensor is undetected
      //}
    }
  }

  // センサ2検知時
  if (digitalRead(SENSOR2) == LOW) {
    delay(2);
    if (digitalRead(SENSOR2) == LOW) {
      // ステート変更
      state = 0;
      // CAN通信でボールが落ちたことを通知
      CAN_DATA[0] = JUMPER_DROP;
      CAN0.sendMsgBuf(MYID + 0x100, 0, 1, CAN_DATA);
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

  // ボール上がった時
  if (state == 1) {
    // LED表示（現在の色相の反転色が反時計回りに回る）
    if ((micros() - led_update) > led_update_period) {
      if (blank >= NUMPIXELS) {
        blank = 0;
      }
      //for (int i = NUMPIXELS - 1; i >= 0; i--) {
      for (int i = 0; i < NUMPIXELS; i++) {
        if (i == blank) {
          LEDHSB(i, (hue + 180) % 360, SAT, BRI);
        }
        else {
          LEDHSB(i, hue, SAT, BRI);
        }
      }
      blank++;
      led_update = micros();
    }
  }

  // ボール上がってない時
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
}
#endif
