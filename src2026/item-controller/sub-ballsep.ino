#ifdef BALLSEP

// ボールセパレーターとボールスルーの制御コードは同じ
// ----------------------------------------------------------------------
void loopBallSep() {
  static int hue[3] = {random(360), random(360), random(360)}; // 色相
  static int bri[3] = {0, 0, 0}; // 輝度
  static uint8_t step = 5; // 輝度の変化量
  static int ss[3] = {step, step, step};
  static unsigned long led_update = 0;
  const unsigned long led_update_period = 50;

  if (digitalRead(SENSOR1) == LOW) {
    delay(DEBOUNCE);
    if (digitalRead(SENSOR1) == LOW) {
      // CAN通信でセンサ感知を通知
      CAN_DATA[0] = BALLSEP_SENSE;
      CAN_DATA[1] = 0x01;
      CAN0.sendMsgBuf(MYID + 0x100, 0, 2, CAN_DATA);
      // LEDの制御（乱数で色を変えて，輝度を最大にする）
      hue[0] = random(360);
      bri[0] = 100;
      LEDHSB(0, hue[0], SAT, bri[0]);
      while (digitalRead(SENSOR1) == LOW) {
        // wait for release SENSOR1
      }
    }
  }
  if (digitalRead(SENSOR2) == LOW) {
    delay(DEBOUNCE);
    if (digitalRead(SENSOR2) == LOW) {
      // CAN通信でセンサ感知を通知
      CAN_DATA[0] = BALLSEP_SENSE;
      CAN_DATA[1] = 0x02;
      CAN0.sendMsgBuf(MYID + 0x100, 0, 2, CAN_DATA);
      // LEDの制御（乱数で色を変えて，輝度を最大にする）
      hue[1] = random(360);
      bri[1] = 100;
      LEDHSB(1, hue[1], SAT, bri[1]);
      while (digitalRead(SENSOR2) == LOW) {
        // wait for release SENSOR2
      }
    }
  }
  if (digitalRead(SENSOR3) == LOW) {
    delay(DEBOUNCE);
    if (digitalRead(SENSOR3) == LOW) {
      // CAN通信でセンサ感知を通知
      CAN_DATA[0] = BALLSEP_SENSE;
      CAN_DATA[1] = 0x04;
      CAN0.sendMsgBuf(MYID + 0x100, 0, 2, CAN_DATA);
      // LEDの制御（乱数で色を変えて，輝度を最大にする）
      hue[2] = random(360);
      bri[2] = 100;
      LEDHSB(2, hue[2], SAT, bri[2]);
      while (digitalRead(SENSOR3) == LOW) {
        // wait for release SENSOR3
      }
    }
  }

  // LEDの明るさを変える
  if ((millis() - led_update) > led_update_period) {
    for (int i = 0; i < NUMPIXELS; i++) {
      bri[i] += ss[i];
      if (bri[i] < 0) {
        ss[i]= step;
        bri[i] = 0;
      }
      if (bri[i] > 100) {
        ss[i] = -1 * step;
        bri[i] = 100;
      }
      LEDHSB(i, hue[i], SAT, bri[i]);
    }
    led_update = millis();
  }
}
#endif
