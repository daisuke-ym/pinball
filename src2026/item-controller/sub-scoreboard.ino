#ifdef SCOREBOARD

// ----------------------------------------------------------------------
void loopScoreBoard() {
  static unsigned long s;
  static unsigned long seg_update = 0;
  const unsigned long seg_period = 20;

  if (digitalRead(CAN0_INT) == LOW) {
    CAN0.readMsgBuf(&CANID, &RX_LEN, RX_BUF);
    // SCOREBOARD_SCORE コマンド（スコア受信）を受け取った時の処理
    //   CAN_DATA[0]: コマンド
    //   CAN_DATA[1]: スコア
    //   CAN_DATA[2]: スコア倍率
    if (CANID == MYID && RX_BUF[0] == SCOREBOARD_SCORE) {
      s = RX_BUF[1] * RX_BUF[2];
      TARGET += s;
      Serial.println("SCOREBOARD_SCORE");
      Serial.println(s);
    }
    // SCOREBOARD_CLEAR コマンド（スコアクリア）を受け取った時の処理
    if (CANID == MYID && RX_BUF[0] == SCOREBOARD_CLEAR) {
      Serial.println("SCOREBOARD_CLEAR");
      if (SCORE > HIGHSCORE) {
        Serial.println("HIGHSCORE updated");
        HIGHSCORE = SCORE;
        EEPROM.update(MEMPTR, (byte)(HIGHSCORE & 0xFF));
        EEPROM.update(MEMPTR + 1, (byte)((HIGHSCORE >> 8) & 0xFF));
        EEPROM.update(MEMPTR + 2, (byte)((HIGHSCORE >> 16) & 0xFF));
        EEPROM.update(MEMPTR + 3, (byte)((HIGHSCORE >> 24) & 0xFF));
      }
      SCORE = 0;
      TARGET = 0;
    }
    // SCOREBOARD_CLEAR_HIGHSCORE コマンド（ハイスコアクリア）を受け取った時の処理
    if (CANID == MYID && RX_BUF[0] == SCOREBOARD_CLEAR_HS) {
      Serial.println("SCOREBOARD_CLEAR_HS");
      HIGHSCORE = 0;
      EEPROM.update(MEMPTR, 0);
      EEPROM.update(MEMPTR + 1, 0);
      EEPROM.update(MEMPTR + 2, 0);
      EEPROM.update(MEMPTR + 3, 0);
      HIGHSCORE = EEPROM.read(MEMPTR) +
                (EEPROM.read(MEMPTR + 1) << 8) +
                (EEPROM.read(MEMPTR + 2) << 16) +
                (EEPROM.read(MEMPTR + 3) << 24);
      Serial.print("HIGHSCORE: ");
      Serial.println(HIGHSCORE);
    }
  }

  if ((millis() - seg_update) > seg_period) {
    dispScore();
    seg_update = millis();
  }
}

// ----------------------------------------------------------------------
void dispScore() {
  if (TARGET > SCORE + 1000000) {
    SCORE += 500000;
  }
  if (TARGET > SCORE + 100000) {
    SCORE += 50000;
  }
  if (TARGET > SCORE + 10000) {
    SCORE += 5000;
  }
  if (TARGET > SCORE + 1000) {
    SCORE += 500;
  }
  if (TARGET > SCORE + 100) {
    SCORE += 50;
  }
  if (TARGET > SCORE + 10) {
    SCORE += 5;
  }
  if (TARGET > SCORE) {
    SCORE++;
  }
  dispScore7seg(SCORE);
}

// ----------------------------------------------------------------------
void dispScore7seg(long value) {
  long v = value;
  long vh = HIGHSCORE;
  long dig[8 * 2];
  int i, j;

  //Serial.println(v);
  // ハイスコア（上段）の処理
  for (i = 15; i >= 8; i--) {
    dig[i] = int(vh / pow(10, i - 8));
    vh -= dig[i] * pow(10, i - 8);
  }
  // スコア（下段）の処理
  for (i = 7; i >= 0; i--) {
    dig[i] = int(v / pow(10, i));
    v -= dig[i] * pow(10, i);
  }
  //Serial.println();

  digitalWrite(LATCH, LOW);
  // ハイスコア（上段）の処理
  for (i = 15; i > 8; i--) {
    if (dig[i] == 0) {
      SPI.transfer(0x0);
    }
    else {
      break;
    }
  }
  for (j = i; j >= 8; j--) {
    SPI.transfer(SC_DIGITS[dig[j]]);
  }
  // スコア（下段）の処理
  for (i = 7; i > 0; i--) {
    if (dig[i] == 0) {
      SPI.transfer(0x0);
    }
    else {
      break;
    }
  }
  for (j = i; j >= 0; j--) {
    SPI.transfer(SC_DIGITS[dig[j]]);
  }
  digitalWrite(LATCH, HIGH);
}

#endif
