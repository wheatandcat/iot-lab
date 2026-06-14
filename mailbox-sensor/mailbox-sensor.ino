#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "secrets.h"
#define ENABLE_SLEEP true

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

  if (wakeupReason == ESP_SLEEP_WAKEUP_GPIO) {
    Serial.println("GPIO Wakeup!");

    connectWiFi();

    sendDiscordMessage("📬 郵便受けが開きました");
  }

  gpio_wakeup_enable(
      GPIO_NUM_4,
      GPIO_INTR_HIGH_LEVEL);

  esp_sleep_enable_gpio_wakeup();

  if (ENABLE_SLEEP) {
    Serial.println("Sleeping...");
    delay(100);
    esp_deep_sleep_start();
  } else {
    Serial.println("Sleep disabled. Debug mode.");
  }
}

void loop() {
}