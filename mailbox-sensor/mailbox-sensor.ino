/**
 * @brief Deep Sleepの起床確認
 *
 * 磁石を外すと起床する
 */

#include <esp_sleep.h>
#define ENABLE_SLEEP false

const int reedPin = D2;

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

  Serial.println("Going to sleep in 5 sec...");
  delay(5000);

  // D2 = HIGHになったら起床
  gpio_wakeup_enable(GPIO_NUM_4, GPIO_INTR_HIGH_LEVEL);

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