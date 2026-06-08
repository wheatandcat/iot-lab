#include <WiFi.h>
#include <HTTPClient.h>
#include "secrets.h"

/**
 * @brief リードスイッチの状態変化をDiscordへ通知する
 *
 * @example
 * 磁石を離して OPEN になったタイミングでDiscordに通知する。
 */
const int reedPin = D2;

int previousState = HIGH;

/**
 * @brief Wi-Fiへ接続する
 */
void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
}

/**
 * @brief Discord Webhookへメッセージを送信する
 *
 * @param message Discordに送る本文
 */
void sendDiscordMessage(const String& message) {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  HTTPClient http;
  http.begin(DISCORD_WEBHOOK_URL);
  http.addHeader("Content-Type", "application/json");

  String payload = "{\"content\":\"" + message + "\"}";

  int httpCode = http.POST(payload);

  Serial.print("Discord response: ");
  Serial.println(httpCode);

  http.end();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(reedPin, INPUT_PULLUP);

  connectWiFi();

  previousState = digitalRead(reedPin);

  Serial.println("Mailbox sensor started");
}

void loop() {
  int currentState = digitalRead(reedPin);

  if (currentState != previousState) {
    if (currentState == LOW) {
      Serial.println("CLOSED");
    } else {
      Serial.println("OPEN");
      sendDiscordMessage("📬 郵便受けが開きました");
    }

    previousState = currentState;
  }

  delay(50);
}