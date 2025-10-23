// ----------------------------------------------------------------------
// *** 各アイテムの各種情報にアクセスするための配列のインデックス ***
// 当てることで得点が入るアイテム
enum _items {
  BALLSEP,  // ボールセパレータ
  BALLTHRU, // ボールスルー

  BUMPER0,  // バンパー左
  BUMPER1,  // バンパー中
  BUMPER2,  // バンパー右
  BUMPER3,  // バンパー上

  SLING0,   // スリングショット左
  SLING1,   // スリングショット右

  STARGET,  // スタンダップターゲット
  DTARGET0, // ドロップターゲット左
  DTARGET1, // ドロップターゲット中
  DTARGET2, // ドロップターゲット右

  JUMPER,   // ジャンパー
} ITEMS;

// ----------------------------------------------------------------------
// *** 各アイテムの得点 ***
const byte SCORE[] = {
   10, // ボールセパレータ
   10, // ボールスルー

   20, // バンパー左
   20, // バンパー中
   20, // バンパー右
   40, // バンパー上

   50, // スリングショット左
   50, // スリングショット右

   30, // スタンダップターゲット
   30, // ドロップターゲット左
   30, // ドロップターゲット中
   30, // ドロップターゲット右

  100, // ジャンパー
};

// ----------------------------------------------------------------------
// *** 各ステージにおける得点の倍率 ***
// ここでの倍率は、実際の得点に掛ける値を10倍したもの
// 例: 1.0倍 -> 10, 1.5倍 -> 15
const byte STAGE_MUL[] = {
  10,  // Stage 1
  12,
  15,
  20,
  25,  // Stage 5
  30,
  35,
  40,
  45,
  50,  // Stage 10
};

// 総ステージ数
const uint8_t NUM_STAGES = sizeof(STAGE_MUL) / sizeof(byte);

// 現在のステージ
uint8_t STAGE = 0;

// ----------------------------------------------------------------------
// ドロップターゲットを上げるまでの時間
const byte DT_TIMEOUT[] = { // 秒
  90, // Stage 1
  80, // Stage 2
  70, // Stage 3
  60, // Stage 4
  50, // Stage 5
  40, // Stage 6
  30, // Stage 7
  20, // Stage 8
  10, // Stage 9
};

// ----------------------------------------------------------------------
/*
  CAN通信のプロトコルについて
  フォーマット
    CAN_ID: 16bit, COMMAND: 8bit, PAYLOAD1: 8bit, PAYLOAD2: 8bit
      CAN_ID: 各アイテムに割り当てられたID兼コマンド
      COMMAND:  コマンド
      PAYLOAD1: パラメータ1
      PAYLOAD2: パラメータ2
*/

// ----------------------------------------------------------------------
// *** 各アイテムの CAN ID ***
// ITEMNAME_RX: ITEM <-- MEGA2560
// ITEMNAME_TX: ITEM --> MEGA2560
// 送受信するもの
const unsigned long OUTHOLE_RX    = 0x010; // アウトホール
const unsigned long OUTHOLE_TX    = 0x110; // アウトホール
const unsigned long EXTRABALL_RX  = 0x011; // エクストラボール
const unsigned long EXTRABALL_TX  = 0x111; // エクストラボール
const unsigned long TIMERBOARD_RX = 0x020; // タイマー
const unsigned long TIMERBOARD_TX = 0x120; // タイマー
const unsigned long DTARGET0_RX   = 0x030; // 左ドロップターゲット
const unsigned long DTARGET0_TX   = 0x130; // 左ドロップターゲット
const unsigned long DTARGET1_RX   = 0x031; // 中ドロップターゲット
const unsigned long DTARGET1_TX   = 0x131; // 中ドロップターゲット
const unsigned long DTARGET2_RX   = 0x032; // 右ドロップターゲット
const unsigned long DTARGET2_TX   = 0x132; // 右ドロップターゲット
const unsigned long JUMPER_RX     = 0x040; // ジャンパー
const unsigned long JUMPER_TX     = 0x140; // ジャンパー
// 送信だけするもの
const unsigned long BALLSEP_TX    = 0x150; // ボールセパレータ
const unsigned long BALLTHRU_TX   = 0x151; // ボールスルー
const unsigned long BUMPER0_TX    = 0x160; // 左バンパー
const unsigned long BUMPER1_TX    = 0x161; // 中バンパー
const unsigned long BUMPER2_TX    = 0x162; // 右バンパー
const unsigned long BUMPER3_TX    = 0x163; // 上バンパー
const unsigned long SLING0_TX     = 0x170; // 左スリングショット
const unsigned long SLING1_TX     = 0x171; // 右スリングショット
const unsigned long STARGET_TX    = 0x180; // スタンダップターゲット
// 受信だけするもの
const unsigned long SCOREBOARD_RX = 0x090; // 得点板
const unsigned long LEDBOARD0_RX  = 0x091; // 30連LEDコントローラ

// ----------------------------------------------------------------------
// *** コマンド ***
// アウトホール
const byte OUTHOLE_TELEMETRY      = 0x00; // 待機中（ボールを打ち出した後の待機状態）
const byte OUTHOLE_WAIT_START     = 0x01; // スタート待機中
const byte OUTHOLE_PUSH_START     = 0x02; // スタートボタンが押された（プレイ中：ボールを打ち出したとみなす）
const byte OUTHOLE_DROP_BALL      = 0x04; // ボールが落ちた
const byte OUTHOLE_WAIT_EXTRABALL = 0x08; // エクストラボールが落ちてくるのを待機中
// エクストラボール
const byte EXTRABALL_TELEMETRY = 0x00; // 待機中
const byte EXTRABALL_SHOOT     = 0x01; // エクストラボール打ち出し中
// タイマー
const byte TIMER_TELEMETRY = 0x00; // 待機中
const byte TIMER_COUNTDOWN = 0x01; // カウントダウン中
const byte TIMER_TIMEUP    = 0x02; // タイムアップ
const byte TIMER_SET       = 0x10; // 時間設定＆スタート
const byte TIMER_STOP      = 0x20; // カウントダウン停止
const byte TIMER_STAGE_NUM = 0x40; // ステージ番号送信
// ドロップターゲット
const byte DTARGET_TELEMETRY = 0x00; // 待機中
const byte DTARGET_HIT       = 0x01; // ボールが当たった
const byte DTARGET_UP        = 0x10; // ターゲットが上がっている・上げる
const byte DTARGET_DOWN      = 0x20; // ターゲットが下がっている・下げる
// ジャンパー
const byte JUMPER_TELEMETRY  = 0x00; // 待機中
const byte JUMPER_BALL_SENSE = 0x01; // ボールを感知した
const byte JUMPER_BALL_SHOOT = 0x02; // ボールを打ち上げた
const byte JUMPER_BALL_DROP  = 0x04; // ボールが落ちた
// ボールセパレータ
const byte BALLSEP_TELEMETRY = 0x00; // 待機中
const byte BALLSEP_HIT       = 0x01; // ボールが当たった
// ボールスルー
const byte BALLTHRU_TELEMETRY = 0x00; // 待機中
const byte BALLTHRU_HIT       = 0x01; // ボールが当たった
// バンパー
const byte BUMPER_TELEMETRY = 0x00; // 待機中
const byte BUMPER_HIT       = 0x01; // ボールが当たった
// スリングショット
const byte SLING_TELEMETRY = 0x00; // 待機中
const byte SLING_HIT       = 0x01; // ボールが当たった
// スタンダップターゲット
const byte STARGET_TELEMETRY = 0x00; // 待機中
const byte STARGET_HIT       = 0x01; // ボールが当たった
// 得点板
const byte SCOREBOARD_TELEMETRY = 0x00; // 待機中
const byte SCOREBOARD_SCORE     = 0x10; // 得点表示
const byte SCOREBOARD_CLEAR     = 0x20; // 得点クリア
// 30連LEDコントローラ
const byte LED30_PAT1 = 0x00; // パターン1
const byte LED30_PAT2 = 0x01; // パターン2
const byte LED30_PAT3 = 0x02; // パターン3
const byte LED30_OFF  = 0x04;  // 消灯
