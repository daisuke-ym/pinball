#ifdef BALLSEP

/*
  ボールセパレーターとボールスルーの制御コードは同じ

  ボールの速度が速いとdigitalRead()では検知が間に合わないことがあるので、
  割り込みで検知するようにしているが、速度によっては検知できない時がある。
  センサー回路の応答限界かもしれない。
*/

// ----------------------------------------------------------------------
volatile bool Sensor1Detected = false;
volatile bool Sensor2Detected = false;
volatile bool Sensor3Detected = false;

// ----------------------------------------------------------------------
ISR(PORTA_PORT_vect) {
  uint8_t flags = PORTA.INTFLAGS;
  if (flags & PIN5_bm) {
    Sensor1Detected = true;
  }
  if (flags & PIN6_bm) {
    Sensor2Detected = true;
  }
  if (flags & PIN7_bm) {
    Sensor3Detected = true;
  }
  PORTA.INTFLAGS = flags;
}

// ----------------------------------------------------------------------
void loopBallSep() {
  static int hue[3] = {random(360), random(360), random(360)}; // 色相
  static int bri[3] = {0, 0, 0}; // 輝度
  static uint8_t step = 5; // 輝度の変化量
  static int ss[3] = {step, step, step};
  static unsigned long led_update = 0;
  const unsigned long led_update_period = 50;

  if (Sensor1Detected) {
    CAN_DATA[0] = BALLSEP_SENSE;
    CAN_DATA[1] = 0x01;
    CAN0.sendMsgBuf(MYID + 0x100, 0, 2, CAN_DATA);
    hue[0] = random(360);
    bri[0] = 100;
    LEDHSB(0, hue[0], SAT, bri[0]);
    noInterrupts();
    Sensor1Detected = false;
    interrupts();
  }
  if (Sensor2Detected) {
    CAN_DATA[0] = BALLSEP_SENSE;
    CAN_DATA[1] = 0x02;
    CAN0.sendMsgBuf(MYID + 0x100, 0, 2, CAN_DATA);
    hue[1] = random(360);
    bri[1] = 100;
    LEDHSB(1, hue[1], SAT, bri[1]);
    noInterrupts();
    Sensor2Detected = false;
    interrupts();
  }
  if (Sensor3Detected) {
    CAN_DATA[0] = BALLSEP_SENSE;
    CAN_DATA[1] = 0x04;
    CAN0.sendMsgBuf(MYID + 0x100, 0, 2, CAN_DATA);
    hue[2] = random(360);
    bri[2] = 100;
    LEDHSB(2, hue[2], SAT, bri[2]);
    noInterrupts();
    Sensor3Detected = false;
    interrupts();
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
