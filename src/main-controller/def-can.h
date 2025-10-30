/*
  def-can.h - CAN通信の定義ファイル
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
const byte OUTHOLE_GAME_START     = 0x02; // ゲーム開始（プレイ中：ボールを打ち出したとみなす）
const byte OUTHOLE_DROP_BALL      = 0x04; // ボールが落ちた
const byte OUTHOLE_DROP_EXTRABALL = 0x08; // エクストラボールが落ちた
const byte OUTHOLE_WAIT_BALL      = 0x10; // ボールが落ちてくるのを待て
const byte OUTHOLE_WAIT_EXTRABALL = 0x20; // エクストラボールが落ちてくるのを待て
// エクストラボール
const byte EXTRABALL_TELEMETRY = 0x00; // 待機中
const byte EXTRABALL_READY     = 0x01; // エクストラボール準備完了
const byte EXTRABALL_SHOOT     = 0x10; // エクストラボールを打ち出せ
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
const byte SCOREBOARD_CLEAR_HIGHSCORE = 0x40; // ハイスコアクリア
// 30連LEDコントローラ
const byte LED30_PAT1 = 0x00; // パターン1
const byte LED30_PAT2 = 0x01; // パターン2
const byte LED30_PAT3 = 0x02; // パターン3
const byte LED30_OFF  = 0x04;  // 消灯
