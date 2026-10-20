#ifdef SLING

// ----------------------------------------------------------------------
void loopSlingShot() {
  static int hue = random(360); // LEDの色相
  static int bri = 100; // LEDの明度
  const int step = 5; // 明度の変化速度
  static int ss = step; // 明度の変化度
  static unsigned long led_update = 0;
  const unsigned long led_update_period = 50;

  if (digitalRead(SENSOR1) == LOW) {
    delay(DEBOUNCE);
    if (digitalRead(SENSOR1) == LOW) {
      // CAN通信でセンサ感知を通知
      CAN_DATA[0] = SLING_SENSE;
      CAN0.sendMsgBuf(MYID + 0x100, 0, 1, CAN_DATA);
      // LEDの制御（乱数で色を変えて，輝度を最大にする）
      hue = random(360);
      bri = 100;
      LEDHSB(0, hue, SAT, bri);
      // ソレノイドを動作
      digitalWrite(SOL1, HIGH);
      delay(10);
      digitalWrite(SOL1, LOW);
      delay(DEADTIME);
      //while (digitalRead(SENSOR1) == LOW) {
        // wait for release SENSOR1
      //}
    }
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
#endif
