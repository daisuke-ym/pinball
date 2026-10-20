#ifdef LED30

#define HUE10  10 // 色相の変化度（粗目）
#define HUE30  30 // 色相の変化度（中目）
#define HUE60  60 // 色相の変化度（細目）
// ----------------------------------------------------------------------
void loopLED30() {
  static uint8_t pattern = 1; // 点灯パターン

  // CANメッセージ受信処理
  if (digitalRead(CAN0_INT) == LOW) {
    CAN0.readMsgBuf(&CANID, &RX_LEN, RX_BUF);
    if (CANID == MYID && RX_BUF[0] == LED30_PATTERN) {
      pattern = RX_BUF[1];
      Serial.print("LED30 pattern: ");
      Serial.println(pattern);
    }
  }

  // 点灯パターンの切り替え
  switch (pattern) {
    case 1:
      ledPattern1();
      break;
    case 2:
      ledPattern2();
      break;
    case 3:
      ledPattern3();
      break;
    case 4:
      rainbow_down2up(30, 2);
      break;
    case 5:
      rainbow_up2down(30, 2);
      break;
    case 6:
      rainbow_left2right(30, 2);
      break;
    default:
      ledPattern1();
      break;
  }
}

// ----------------------------------------------------------------------
void rainbow_down2up(int step, byte times) {
  uint16_t hue[60];

  for (byte t = 0; t < times; t++) {
    // 色を作る
    for (byte i = 0; i < step; i++) {
      hue[i] = (360 / step) * i;
    }
    // 色を変化させる
    for (byte j = 0; j < step; j++) {
      for (byte i = 0; i < 10; i++) {
        LEDHSB(9 - i, hue[(i + j) % step], SAT, BRI);
        LEDHSB(9 - i + 10, hue[(i + j) % step], SAT, BRI);
        LEDHSB(9 - i + 20, hue[(i + j) % step], SAT, BRI);
      }
      delay(100);
    }
  }
}

// ----------------------------------------------------------------------
void rainbow_up2down(int step, byte times) {
  uint16_t hue[60];
  
  for (byte t = 0; t < times; t++) {
    // 色を作る
    for (byte i = 0; i < step; i++) {
      hue[i] = (360 / step) * i;
    }
    // 色を変化させる
    for (byte j = 0; j < step; j++) {
      for (byte i = 0; i < 10; i++) {
        LEDHSB(i, hue[(i + j) % step], SAT, BRI);
        LEDHSB(i + 10, hue[(i + j) % step], SAT, BRI);
        LEDHSB(i + 20, hue[(i + j) % step], SAT, BRI);
      }
      delay(100);
    }
  }
}

// ----------------------------------------------------------------------
void rainbow_left2right(int step, byte times) {
  uint16_t hue[60];
  
  for (byte t = 0; t < times; t++) {
    // 色を作る
    for (byte i = 0; i < step; i++) {
      hue[i] = (360 / step) * i;
    }
    // 色を変化させる
    for (byte j = 0; j < step; j++) {
      for (byte i = 0; i < 3; i++) {
        for (byte k = 0; k < 10; k++) {
          LEDHSB(k, hue[(i + j) % step], SAT, BRI);
        }
        for (byte k = 10; k < 20; k++) {
          LEDHSB(k, hue[(i + j + 1) % step], SAT, BRI);
        }
        for (byte k = 20; k < 30; k++) {
          LEDHSB(k, hue[(i + j + 2) % step], SAT, BRI);
        }
      }
      delay(100);
    }
  }
}

// ----------------------------------------------------------------------
// 点灯パターン0
//   全部同時に虹色に変化
void ledPattern0() {
  static byte bri = 100;

  for (int h = 0; h < 360; h += 3) {
    for (byte i = 0; i < NUMPIXELS; i++) {
      LEDHSB(i, h, SAT, bri);
    }
    delay(50);
  }
}

// ----------------------------------------------------------------------
// 点灯パターン1
//   下から上に虹色移動（粗目）
void ledPattern1() {
  uint16_t hue[60];
  
  for (byte i = 0; i < HUE10; i++) {
    hue[i] = (360 / HUE10) * i;
  }
  for (byte j = 0; j < HUE10; j++) {
    for (byte i = 0; i < 10; i++) {
      LEDHSB(9 - i, hue[(i + j) % HUE10], SAT, BRI);
      LEDHSB(9 - i + 10, hue[(i + j) % HUE10], SAT, BRI);
      LEDHSB(9 - i + 20, hue[(i + j) % HUE10], SAT, BRI);
    }
    delay(100);
  }
}

// ----------------------------------------------------------------------
// 点灯パターン2
//   上から下に虹色移動（粗目）
void ledPattern2() {
  uint16_t hue[60];
  
  for (byte i = 0; i < HUE10; i++) {
    hue[i] = (360 / HUE10) * i;
  }
  for (byte j = 0; j < HUE10; j++) {
    for (byte i = 0; i < 10; i++) {
      LEDHSB(i, hue[(i + j) % HUE10], SAT, BRI);
      LEDHSB(i + 10, hue[(i + j) % HUE10], SAT, BRI);
      LEDHSB(i + 20, hue[(i + j) % HUE10], SAT, BRI);
    }
    delay(100);
  }
}

// ----------------------------------------------------------------------
// 点灯パターン3
//   下から上に虹色移動（中目）
void ledPattern3() {
  uint16_t hue[60];

  for (byte i = 0; i < HUE30; i++) {
    hue[i] = (360 / HUE30) * i;
  }
  for (byte j = 0; j < HUE30; j++) {
    for (byte i = 0; i < 10; i++) {
      LEDHSB(9 - i, hue[(i + j) % HUE30], SAT, BRI);
      LEDHSB(9 - i + 10, hue[(i + j) % HUE30], SAT, BRI);
      LEDHSB(9 - i + 20, hue[(i + j) % HUE30], SAT, BRI);
    }
    delay(100);
  }
}
#endif
