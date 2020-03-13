#include "bouncer.h"
#include "blinker.h"

#include <ESP8266WiFi.h>

extern "C" {
#include "gpio.h"
}


PinBouncer coldBouncer(COLD_PIN_NUMBER, "Cold", 100);
PinBouncer hotBouncer(HOT_PIN_NUMBER, "Hot", 100);

Blinker greenBlinker(GREEN_LED_PIN_NUMBER);
Blinker redBlinker(RED_LED_PIN_NUMBER);

unsigned long m_lastTimestamp = 0;

void wakeupFromMotion(void)
{
    wifi_fpm_close();
    wifi_set_opmode(STATION_MODE);
    wifi_station_connect();

    pinMode(GREEN_LED_PIN_NUMBER, OUTPUT);
    pinMode(RED_LED_PIN_NUMBER, OUTPUT);

    greenBlinker.setMode(Blinker::Error);
    redBlinker.setMode(Blinker::Error);

    Serial.printf("[%lu] Woke up from sleep\n", millis());

    m_lastTimestamp = millis();
}

void sleepNow()
{
    Serial.printf("[%lu] going to deep sleep...\n", millis());
    ESP.deepSleep(0);

    Serial.printf("[%lu] going to light sleep...\n", millis());

    greenBlinker.setMode(Blinker::Off);
    redBlinker.setMode(Blinker::Off);

    digitalWrite(GREEN_LED_PIN_NUMBER, LOW);
    digitalWrite(RED_LED_PIN_NUMBER, LOW);

    pinMode(GREEN_LED_PIN_NUMBER, INPUT);
    pinMode(RED_LED_PIN_NUMBER, INPUT);

    wifi_station_disconnect();
    wifi_set_opmode(NULL_MODE);
    wifi_fpm_set_sleep_type(LIGHT_SLEEP_T); //light sleep mode
    gpio_pin_wakeup_enable(COLD_PIN_NUMBER, GPIO_PIN_INTR_LOLEVEL); //set the interrupt to look for HIGH pulses on Pin 0 (the PIR).
    gpio_pin_wakeup_enable(HOT_PIN_NUMBER, GPIO_PIN_INTR_LOLEVEL); //set the interrupt to look for HIGH pulses on Pin 0 (the PIR).
    wifi_fpm_open();
    delay(100);
    wifi_fpm_set_wakeup_cb(wakeupFromMotion); //wakeup callback
    wifi_fpm_do_sleep(0xFFFFFFF);
    // wifi_fpm_do_sleep(3000000);
    delay(100);
}

void setup()
{
    Serial.begin(74880);
    Serial.flush();
    Serial.printf("\r\nJJR Pulser WakeUp Test 2019-08-28 17:09:50\r\n");
    Serial.flush();

    coldBouncer.setup();
    hotBouncer.setup();

    greenBlinker.setup();
    redBlinker.setup();

    greenBlinker.setMode(Blinker::Error);
    redBlinker.setMode(Blinker::Error);

    m_lastTimestamp = millis();
}

void loop()
{
    greenBlinker.work();
    redBlinker.work();

    if ((millis() - m_lastTimestamp) > 5000)
    {
        m_lastTimestamp = millis();
        sleepNow();
    }
}
