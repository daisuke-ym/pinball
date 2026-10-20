#ifdef OUTHOLE

// ----------------------------------------------------------------------
void loopOuthole() {
  static uint8_t state = 0;
  static uint8_t lastState = 10;
  // stateの意味
  // 0:スタートボタン待ち（待機中）  1:ゲーム中（通常ゲーム中）  2:ゲーム中（ボーナスゲーム中）
  // 0 --> 1: スタートボタン押下げによるゲーム開始
  // 1 --> 0: ボールがアウトホールに落下してゲーム終了
  // 1 --> 2: エクストラボールが投入されてボーナスゲーム開始
  // 2 --> 1: エクストラボールがアウトホールに落下して通常ゲームに戻る
  static uint8_t ebState = 0; // 0:エクストラボール待機中  1:エクストラボール打ち出し中
  static unsigned long servoReturnTime = 0; // サーボを中央へ復帰させる時間
  const long servoReturnDelay = 2000; // サーボを中央へ復帰させるまでの時間

  // スタートボタンの処理
  if (digitalRead(SENSOR2) == LOW && digitalRead(SENSOR3) == LOW) {
    delay(DEBOUNCE);
    if (digitalRead(SENSOR2) == LOW && digitalRead(SENSOR3) == LOW) {
      state = 1;
      ebState = 0; // ゲーム開始時はエクストラボール待機中に設定
      // スタート位置にボールを打ち出す
      digitalWrite(SOL1, HIGH);
      delay(10);
      digitalWrite(SOL1, LOW);
    }
  }

  // アウトホールの処理
  if (digitalRead(SENSOR1) == LOW) {
    delay(DEBOUNCE);
    if (digitalRead(SENSOR1) == LOW) {
      if (ebState == 0) {
        state = 0; // ゲーム終了
        // ボール落下を送信する
        CAN_DATA[0] = OUTHOLE_DROP_BALL;
        CAN0.sendMsgBuf(MYID + 0x100, 0, 1, CAN_DATA);
        // スタート側にボールを流す
        SV.write(SV_START);
        servoReturnTime = millis() + servoReturnDelay;
      }
      else {
        ebState = 0; // エクストラボールが落下したので通常ゲームに移行
        state = 1; // 通常ゲームに移行
        // エクストラボール落下を送信する
        CAN_DATA[0] = OUTHOLE_DROP_EXTRABALL;
        CAN0.sendMsgBuf(MYID + 0x100, 0, 1, CAN_DATA);
        // エクストラボール側にボールを流す
        SV.write(SV_EXTRA);
        servoReturnTime = millis() + servoReturnDelay;
      }
      while (digitalRead(SENSOR1) == LOW) {
        delay(10);
      }
    }
  }

  // サーボを中央へ復帰させる処理
  if (servoReturnTime > 0 && millis() >= servoReturnTime) {
    SV.write(SV_CENTER);
    servoReturnTime = 0;
  }

  // CANメッセージ受信処理
  if (digitalRead(CAN0_INT) == LOW) {
    // メッセージ受信
    CAN0.readMsgBuf(&CANID, &RX_LEN, RX_BUF);
    // OUTHOLE_WAIT_BALL コマンドを受け取った時の処理
    // 通常このコマンドが送られてくることはない
    if (CANID == MYID && RX_BUF[0] == OUTHOLE_WAIT_BALL) {
      state = 1;
      ebState = 0;
    }
    // OUTHOLE_WAIT_EXTRABALL コマンドを受け取った時の処理
    if (CANID == MYID && RX_BUF[0] == OUTHOLE_WAIT_EXTRABALL) {
      state = 2;
      ebState = 1;
    }
  }

  // 状態が変化した場合にCANメッセージを送信する処理
  if (state != lastState) {
    switch (state) {
      case 0:
        CAN_DATA[0] = OUTHOLE_WAIT_START;
        break;
      case 1:
        CAN_DATA[0] = OUTHOLE_WAIT_BALL;
        break;
      case 2:
        CAN_DATA[0] = OUTHOLE_WAIT_EXTRABALL;
        break;
    }
    CAN0.sendMsgBuf(MYID + 0x100, 0, 1, CAN_DATA);
    lastState = state;
  }
}

// ----------------------------------------------------------------------
void testOutholeServo() {
  if (digitalRead(SENSOR1) == LOW) {
    delay(20);
    if (digitalRead(SENSOR1) == LOW) {
      SV.write(SV_EXTRA);
    }
  }
  if (digitalRead(SENSOR2) == LOW) {
    delay(20);
    if (digitalRead(SENSOR2) == LOW) {
      SV.write(90);
    }
  }
  if (digitalRead(SENSOR3) == LOW) {
    delay(20);
    if (digitalRead(SENSOR3) == LOW) {
      SV.write(SV_START);
    }
  }
}
#endif
