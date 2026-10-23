#ifdef FLIPPER

// ----------------------------------------------------------------------
void loopFlipper() {
  static int state = LOW;
  static int lastState = HIGH;
  static unsigned long stateChangeCount = 0;
  static unsigned long reportStartedAt = 0;
  static unsigned long recentCounts[5] = {};
  static byte recentCountIndex = 0;
  static byte recentCountSize = 0;
  const byte recentCountMax = 5;
  static float averageCount = 0;
  const float overLoadAverage = 7.0; // この回数を超えると過負荷とする
  const unsigned long coolingTime = 3000; // 過負荷になったとき停止する時間(ms)
  const unsigned long reportPeriod = 1000; // ON/OFF回数報告間隔(ms)
  const unsigned int oneshot = 50;
  const unsigned int deadtime = 30;
  static int hue = 0;
  static int bri = 100; // 輝度
  static uint8_t step = 5; // 輝度の変化量
  static int ss = step;
  static unsigned long led_update = 0;
  const unsigned long led_update_period = 50;

  // フリッパー操作スイッチの読み込み
  // ON/OFFの両方でチャタリング対策する
  if (digitalRead(SENSOR1) == LOW) {
    delay(20);
    if (digitalRead(SENSOR1) == LOW) {
      state = LOW;
    }
  }
  if (digitalRead(SENSOR1) == HIGH) {
    delay(20);
    if (digitalRead(SENSOR1) == HIGH) {
      state = HIGH;
    }
  }

  if (state != lastState) {
    stateChangeCount++;
    if (state == LOW) {
      // フリッパー上げ
      digitalWrite(SOL2, LOW);
      digitalWrite(SOL1, HIGH);
      delay(oneshot);
      digitalWrite(SOL1, LOW);
      delay(deadtime);
    }
    else {
      // フリッパー下げ
      digitalWrite(SOL1, LOW);
      digitalWrite(SOL2, HIGH);
      delay(oneshot);
      digitalWrite(SOL2, LOW);
      delay(deadtime);
    }
  }

  lastState = state;

  unsigned long now = millis();
  if (now - reportStartedAt >= reportPeriod) {
    recentCounts[recentCountIndex] = stateChangeCount;
    recentCountIndex = (recentCountIndex + 1) % recentCountMax;
    if (recentCountSize < recentCountMax) {
      recentCountSize++;
    }

    Serial.print("State changes per interval (last ");
    Serial.print(recentCountSize);
    Serial.print("): ");
    byte oldestIndex = (recentCountIndex + recentCountMax - recentCountSize) % recentCountMax;
    unsigned long totalCount = 0;
    for (byte i = 0; i < recentCountSize; i++) {
      if (i > 0) {
        Serial.print(", ");
      }
      unsigned long count = recentCounts[(oldestIndex + i) % recentCountMax];
      Serial.print(count);
      totalCount += count;
    }
    averageCount = (float)totalCount / recentCountSize;
    Serial.print(" | Average: ");
    Serial.print(averageCount);
    Serial.print(" | Hue: ");
    hue = mapFloat(averageCount, 0.0, overLoadAverage, LED_BLUE, LED_RED);
    if (hue <= 0.0) {
      hue = LED_RED;
    }
    Serial.print(hue);
    Serial.println();

    stateChangeCount = 0;
    reportStartedAt = now;
  }

  // 操作回数が過負荷閾値を超えた時
  if (averageCount >= overLoadAverage) {
    Serial.println("over load");
    for (byte i = 0; i < recentCountMax; i++) {
      recentCounts[i] = 1;
    }
    // LEDを点滅させて警告する（この間は操作無効）
    unsigned long coolingStartedAt = millis();
    while (millis() - coolingStartedAt < coolingTime) {
      LEDHSB(0, LED_RED, SAT, bri);
      delay(100);
      LEDHSB(0, LED_RED, SAT, 0);
      delay(100);
    }
  }

  // LEDの明るさを変える
  if ((millis() - led_update) > led_update_period) {
    /*
    bri += ss;
    if (bri < 0) {
      ss = step;
      bri = 0;
    }
    if (bri > 100) {
      ss = -1 * step;
      bri = 100;
    }
    */
    LEDHSB(0, hue, SAT, bri);
    led_update = millis();
  }
}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return ((float)x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
#endif
