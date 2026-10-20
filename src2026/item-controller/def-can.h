/*
  CAN通信に関する定義
*/

#ifndef DEF_CAN_H
#define DEF_CAN_H

// ----------------------------------------------------------------------
// *** 各アイテムのCANID ***
// 命名規則（MAINを基準にTX/RXを付与）
//   MAIN --> ITEM:  ITEM_TX
//   ITEM --> MAIN:  ITEM_RX
const unsigned long OUTHOLE_TX    = 0x010; // アウトホール
const unsigned long OUTHOLE_RX    = 0x110;
const unsigned long EXTRABALL_TX  = 0x011; // エクストラボール
const unsigned long EXTRABALL_RX  = 0x111;
const unsigned long TIMER7SEG_TX  = 0x020; // タイマー
const unsigned long TIMER7SEG_RX  = 0x120;
const unsigned long DTARGET0_TX   = 0x030; // 左ドロップターゲット
const unsigned long DTARGET0_RX   = 0x130;
const unsigned long DTARGET1_RX   = 0x131; // 中央ドロップターゲット
const unsigned long DTARGET1_TX   = 0x031;
const unsigned long DTARGET2_TX   = 0x032; // 右ドロップターゲット
const unsigned long DTARGET2_RX   = 0x132;
const unsigned long JUMPER_TX     = 0x040; // ジャンパー
const unsigned long JUMPER_RX     = 0x140;
const unsigned long BALLSEP_TX    = 0x050; // ボールセパレーター
const unsigned long BALLSEP_RX    = 0x150;
const unsigned long BALLTHRU_TX   = 0x051; // ボールスルー
const unsigned long BALLTHRU_RX   = 0x151;
const unsigned long BUMPER0_TX    = 0x060; // 左バンパー
const unsigned long BUMPER0_RX    = 0x160;
const unsigned long BUMPER1_TX    = 0x061; // 中央バンパー
const unsigned long BUMPER1_RX    = 0x161;
const unsigned long BUMPER2_TX    = 0x062; // 右バンパー
const unsigned long BUMPER2_RX    = 0x162;
const unsigned long BUMPER3_TX    = 0x063; // 上バンパー
const unsigned long BUMPER3_RX    = 0x163;
const unsigned long SLING0_TX     = 0x070; // 左スリング
const unsigned long SLING0_RX     = 0x170;
const unsigned long SLING1_TX     = 0x071; // 右スリング
const unsigned long SLING1_RX     = 0x171;
const unsigned long STARGET_TX    = 0x080; // スタンダップターゲット
const unsigned long STARGET_RX    = 0x180;
const unsigned long SCOREBOARD_TX = 0x090; // 得点板
const unsigned long SCOREBOARD_RX = 0x190;
const unsigned long LEDBOARD_TX   = 0x091; // 30連LED
const unsigned long LEDBOARD_RX   = 0x191;

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
// *** コマンド ***
// 共通
const byte ITEM_TELEMETRY = 0x00; // テレメトリー
const byte SEND_YOUR_NAME = 0xff; // 名前要求
const byte ITEM_RESET     = 0xaa; // ソフトウェアリセット
// アウトホール
const byte OUTHOLE_WAIT_START     = 0x01; // スタートボタン押下げ待ち
const byte OUTHOLE_WAIT_BALL      = 0x02; // ボール落下待ち（通常ゲーム中）
const byte OUTHOLE_WAIT_EXTRABALL = 0x04; // エクストラボール落下待ち（ボーナスゲーム中）
const byte OUTHOLE_DROP_BALL      = 0x10; // ボールが落ちた（ゲーム終了）
const byte OUTHOLE_DROP_EXTRABALL = 0x20; // エクストラボールが落ちた（ボーナスゲーム終了）
// エクストラボール
const byte EXTRABALL_READY     = 0x01; // エクストラボール待機中
const byte EXTRABALL_SHOOTED   = 0x02; // エクストラボールが打ち出された
const byte EXTRABALL_SHOOT     = 0x10; // エクストラボールを打ち出せ
// タイマー
const byte TIMER_START     = 0x01; // タイマー時間設定・開始
const byte TIMER_STOP      = 0x02; // タイマー停止
const byte TIMER_STAGE_NUM = 0x04; // ステージ番号
const byte TIMER_RUNNING   = 0x10; // タイマー動作中
const byte TIMER_STANDBY   = 0x20; // タイマー待機中
const byte TIMER_TIMEUP    = 0x40; // タイマー時間切れ
// ドロップターゲット
const byte DTARGET_HIT       = 0x01; // ボールが当たった
const byte DTARGET_LIFT      = 0x10; // ターゲットが上がっている・上げる
const byte DTARGET_DROP      = 0x20; // ターゲットが下がっている・下げる
// ジャンパー
const byte JUMPER_TELEMETRY = 0x00; // テレメトリー
const byte JUMPER_SENSE     = 0x01; // ボールを検知した
const byte JUMPER_SHOOT     = 0x02; // ボールを打ち上げた
const byte JUMPER_DROP      = 0x04; // ボールが落ちた
// ボールセパレーター
const byte BALLSEP_TELEMETRY = 0x00; // テレメトリー
const byte BALLSEP_SENSE     = 0x01; // ボールを検知した
// ボールスルー
const byte BALLTHRU_TELEMETRY = 0x00; // テレメトリー
const byte BALLTHRU_SENSE     = 0x01; // ボールを検知した
// バンパー
const byte BUMPER_TELEMETRY = 0x00; // テレメトリー
const byte BUMPER_HIT       = 0x01; // ボールが当たった
// スリング
const byte SLING_TELEMETRY = 0x00; // テレメトリー
const byte SLING_SENSE     = 0x01; // ボールを検知した
// スタンダップターゲット
const byte STARGET_TELEMETRY = 0x00; // テレメトリー
const byte STARGET_HIT       = 0x01; // ボールが当たった
// 得点板
const byte SCOREBOARD_TELEMETRY = 0x00; // テレメトリー
const byte SCOREBOARD_SCORE     = 0x01; // 得点表示
const byte SCOREBOARD_CLEAR     = 0x02; // 得点クリア
const byte SCOREBOARD_CLEAR_HS  = 0x04; // ハイスコアクリア
// 30連LED
const byte LED30_TELEMETRY = 0x00; // テレメトリー
const byte LED30_PAT1      = 0x01; // パターン1
const byte LED30_PAT2      = 0x02; // パターン2
const byte LED30_PAT3      = 0x04; // パターン3
const byte LED30_PAT4      = 0x08; // パターン4
const byte LED30_PAT5      = 0x10; // パターン5
const byte LED30_PAT6      = 0x20; // パターン6
const byte LED30_PAT7      = 0x40; // パターン7
const byte LED30_PAT8      = 0x80; // パターン8

#endif // DEF_CAN_H
