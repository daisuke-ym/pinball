#ifdef BUMPER

// ----------------------------------------------------------------------
void loopBumper() {
  static int hue = random(360);
  static int bri = 0;
  const int step = 5;
  static int ss = step;
  static unsigned long led_update = 0;
  const unsigned long led_update_period = 50;

  // センサ検知
  if (digitalRead(SENSOR1) == LOW) {
    delay(DEBOUNCE);
    if (digitalRead(SENSOR1) == LOW) {
      // CAN通信でセンサ感知を通知
      CAN_DATA[0] = BUMPER_HIT;
      CAN0.sendMsgBuf(MYID + 0x100, 0, 1, CAN_DATA);
      // LEDの制御（乱数で色を変え，輝度を最大にする）
      hue = random(360);
      bri = 100;
      LEDHSB(0, hue, SAT, bri);
      // ソレノイド動作
      digitalWrite(SOL1, HIGH);
      delay(BUMPER_ONESHOT);
      digitalWrite(SOL1, LOW);
      // デッドタイム
      delay(BUMPER_DEADTIME);
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
