#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "secrets.h"
#define ENABLE_SLEEP true
// 一時的なデバッグ用。true にすると起動のたびに wakeup reason を Discord へ送る。
// 起床できているかをログなしで確認するための切り分け。確認後 false に戻す。
#define DEBUG_DISCORD_LOG false

/**
 * @brief リードスイッチの状態変化をDiscordへ通知する
 *
 * @example
 * 磁石を離して OPEN になったタイミングでDiscordに通知する。
 */
const int reedPin = D2;

unsigned long lastNotifyAt = 0;
const unsigned long notifyCooldownMs = 10000;

/**
 * @brief Wi-Fiへ接続する
 *
 * 接続が不安定な場合に備えて、状態とRSSIをログ出力する。
 *
 * @return true 接続成功
 * @return false 接続失敗
 */
bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.disconnect(true);
  delay(1000);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");

  const unsigned long timeoutMs = 30000;
  const unsigned long startedAt = millis();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");

    if (millis() - startedAt > timeoutMs) {
      Serial.println();
      Serial.print("WiFi timeout. status=");
      Serial.println(WiFi.status());
      return false;
    }
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());

  delay(1000);
  return true;
}

/**
 * @brief Discord Webhookへメッセージを送信する
 *
 * HTTPS接続はESP32環境だとたまに失敗するため、最大3回リトライする。
 *
 * @param message Discordに送る本文
 * @return true 送信成功
 * @return false 送信失敗
 *
 * @example
 * bool ok = sendDiscordMessage("📬 郵便受けが開きました");
 */
bool sendDiscordMessage(const String& message) {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  const int maxRetries = 5;

  for (int attempt = 1; attempt <= maxRetries; attempt++) {
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(10000);

    HTTPClient http;
    http.setTimeout(10000);
    http.setReuse(false);

    Serial.printf("Discord attempt: %d\n", attempt);

    if (!http.begin(client, DISCORD_WEBHOOK_URL)) {
      Serial.println("HTTP begin failed");
      http.end();
      delay(1000);
      continue;
    }

    http.addHeader("Content-Type", "application/json");

    String payload = "{\"content\":\"" + message + "\"}";
    int httpCode = http.POST(payload);

    Serial.printf("HTTP Code: %d\n", httpCode);

    if (httpCode == 204) {
      Serial.println("Discord success");
      http.end();
      return true;
    }

    if (httpCode <= 0) {
      Serial.printf(
        "HTTP Error: %s\n",
        http.errorToString(httpCode).c_str()
      );
    } else {
      Serial.printf("Discord HTTP error: %d\n", httpCode);
    }

    http.end();

    delay(1000);
  }

  Serial.println("Discord failed after retries");
  return false;
}

void setup() {
  Serial.begin(115200);
  delay(3000);

  Serial.println();
  Serial.println("Boot!");

  pinMode(reedPin, INPUT_PULLUP);

  esp_sleep_wakeup_cause_t wakeupReason =
      esp_sleep_get_wakeup_cause();

  Serial.print("Wakeup reason: ");
  Serial.println((int)wakeupReason);

#if DEBUG_DISCORD_LOG
  // 起床できているかの切り分け用。reason=7 なら GPIO 起床成功。
  connectWiFi();
  sendDiscordMessage("boot reason=" + String((int)wakeupReason));
#endif

  if (wakeupReason == ESP_SLEEP_WAKEUP_GPIO) {
    Serial.println("GPIO Wakeup!");

    connectWiFi();

    sendDiscordMessage("📬 郵便受けが開きました");
  }

  // ESP32-C3/C6/S3 などでは deep sleep からの GPIO 起床は
  // esp_deep_sleep_enable_gpio_wakeup() を使う。
  // gpio_wakeup_enable() + esp_sleep_enable_gpio_wakeup() は light sleep 専用で
  // deep sleep では起床ソースにならない。
  esp_deep_sleep_enable_gpio_wakeup(
      1ULL << GPIO_NUM_4,
      ESP_GPIO_WAKEUP_GPIO_HIGH);

  // 暫定策: deep sleep 中も内部プルアップ設定を保持させる試み。
  // 磁石を外した時に GPIO4 を HIGH に引き上げて起床させるため。
  // これで起床しない場合は外付け 10kΩ プルアップ抵抗(GPIO4↔3.3V)を入れる。
  gpio_pullup_en(GPIO_NUM_4);
  gpio_hold_en(GPIO_NUM_4);
  gpio_deep_sleep_hold_en();

  if (ENABLE_SLEEP) {
    Serial.println("Sleeping...");
    delay(100);
    esp_deep_sleep_start();
  } else {
    Serial.println("Sleep disabled. Debug mode.");
  }
}

// デバッグモード（ENABLE_SLEEP == false）では deep sleep に入らないため、
// loop() でリードスイッチを監視して通知する。シリアル接続を保ったまま
// Wi-Fi / Discord / センサー配線を検証できる。
void loop() {
  if (ENABLE_SLEEP) {
    return;
  }

  static int lastState = digitalRead(reedPin);
  int state = digitalRead(reedPin);

  // LOW(磁石あり) -> HIGH(磁石なし=OPEN) の立ち上がりで通知
  if (state == HIGH && lastState == LOW) {
    if (millis() - lastNotifyAt > notifyCooldownMs) {
      Serial.println("OPEN detected (debug)");
      sendDiscordMessage("📬 郵便受けが開きました（debug）");
      lastNotifyAt = millis();
    }
  }

  lastState = state;
  delay(50);
}