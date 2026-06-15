# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 概要

ESP32 用の IoT 実験リポジトリ。現状は `mailbox-sensor/` のみで、リードスイッチ（磁気センサー）で郵便受けの開閉を検知し、Discord Webhook へ通知する単一の Arduino スケッチで構成される。

## ビルド / 書き込み

リポジトリにはビルドスクリプトや CI はなく、Arduino IDE もしくは `arduino-cli` で ESP32 ボード向けにコンパイル・書き込みする。`.ino` のファイル名はディレクトリ名（`mailbox-sensor`）と一致させる必要がある（Arduino の制約）。シリアルモニタは `115200` baud。

## セットアップ上の必須事項

- `mailbox-sensor/secrets.h` は **gitignore 済み**で、リポジトリには含まれない。書き込み前に手動で作成する必要がある。定義すべきマクロ:
  - `WIFI_SSID`
  - `WIFI_PASSWORD`
  - `DISCORD_WEBHOOK_URL`

## アーキテクチャ / 動作の要点

`mailbox-sensor/mailbox-sensor.ino` は **deep sleep ベースのイベント駆動**で動作し、`loop()` は使わず処理はすべて `setup()` 内で完結する:

1. `setup()` 冒頭で `esp_sleep_get_wakeup_cause()` を確認し、GPIO 起床（`ESP_SLEEP_WAKEUP_GPIO`）だった場合のみ Wi-Fi 接続 → Discord 通知を実行する。
2. 通知後（または通常起動時）に GPIO 起床を再設定し、`esp_deep_sleep_start()` で再び眠る。
3. リードスイッチは `reedPin = D2`（= `GPIO_NUM_4`）に `INPUT_PULLUP` で接続。`gpio_wakeup_enable(GPIO_NUM_4, GPIO_INTR_HIGH_LEVEL)` により HIGH レベルで起床する（磁石が離れて OPEN になると通知）。

### デバッグ時の注意

- `#define ENABLE_SLEEP true`（ファイル先頭）を `false` にすると deep sleep に入らず動作し続けるデバッグモードになる。シリアルログを追う際はこちらを使う。
- HTTPS 接続は `client.setInsecure()` で証明書検証を省略している。ESP32 では HTTPS POST がたまに失敗するため、`sendDiscordMessage()` は最大 5 回リトライし、成功条件は HTTP `204` のみ。
