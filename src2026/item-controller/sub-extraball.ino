#ifdef EXTRABALL

// ----------------------------------------------------------------------
void loopExtraBall() {
  static uint8_t state = 0; // 0:エクストラボール待機中（SENSOR2検知）    1:エクストラボール打ち出し中（SENSOR2未検知）
  static uint8_t lastState = 1;
  static uint8_t ExtraBallShoot = 0; // 0:エクストラボール待機中    1:エクストラボール打ち出し中
  static int sensor2_state = HIGH;
  static uint8_t shootNextPos = 0; // 0:待機    1:次の位置に打ち出せ
  static unsigned long shootNextPosTime = 0; // 打ち出す時間

  // ボールセンサ1が検知したら次の位置までボールを打ち出す
  if (digitalRead(SENSOR1) == LOW) {
    delay(DEBOUNCE);
    if (digitalRead(SENSOR1) == LOW) {
      shootNextPos = 1;
      shootNextPosTime = millis() + 2000; // 2秒後に次の位置に打ち出す
    }
  }
  // 時間が来たら次の位置に打ち出す
  // delay()で待つと他の処理が止まってしまうので、タイマーで管理する
  if (shootNextPos == 1 && millis() > shootNextPosTime) {
    shootNextPos = 0;
    digitalWrite(SOL1, HIGH);
    delay(10);
    digitalWrite(SOL1, LOW);
  }

  // ボールセンサ2の状態変化を検出
  state = digitalRead(SENSOR2);

  // 状態が変化した場合にCANメッセージを送信する処理
  if (state != lastState) {
    switch (state) {
      case 0:
        CAN_DATA[0] = EXTRABALL_READY;
        break;
      case 1:
        CAN_DATA[0] = EXTRABALL_SHOOTED;
        break;
    }
    CAN0.sendMsgBuf(MYID + 0x100, 0, 1, CAN_DATA);
    lastState = state;
  }

  // CANメッセージ受信処理
  if (digitalRead(CAN0_INT) == LOW) {
    CAN0.readMsgBuf(&CANID, &RX_LEN, RX_BUF);
    // EXTRABALL_SHOOT コマンドを受け取った時の処理
    if (CANID == MYID && RX_BUF[0] == EXTRABALL_SHOOT) {
      // エクストラボールを打ち出す
      digitalWrite(SOL2, HIGH);
      delay(10);
      digitalWrite(SOL2, LOW);
      CAN_DATA[0] = EXTRABALL_SHOOT;
      CAN0.sendMsgBuf(MYID + 0x100, 0, 1, CAN_DATA);
      ExtraBallShoot = 1;
    }
  }
}
#endif
