#include "bouncer.h"
#include "blinker.h"
#include "storage.h"
#include "test.h"
#include "passwords.h"

#include <ESP8266WiFi.h>

PinBouncer coldBouncer(COLD_PIN_NUMBER, "Cold");
PinBouncer hotBouncer(HOT_PIN_NUMBER, "Hot");

Blinker greenBlinker(GREEN_LED_PIN_NUMBER);
Blinker redBlinker(RED_LED_PIN_NUMBER);

bool processBouncer(PinBouncer &pb)
{
    pb.work();

    if (!pb.stable())
        return false;

    if (!pb.newValue())
        return false;

    Serial.printf("%s: %d\n", pb.name(), pb.value());
    pb.resetNewValue();

    return pb.value() == LOW;
}

void setup()
{
    Serial.begin(74880);
    Serial.flush();
    Serial.printf("\r\nJJR Pulser Setup\r\n");
    Serial.print("MAC: ");
    Serial.println(WiFi.macAddress());
    Serial.flush();

    coldBouncer.setup();
    hotBouncer.setup();

    greenBlinker.setup();
    redBlinker.setup();

    DataStorage::setup(JJR_PULSER_WIFI_SSID, JJR_PULSER_WIFI_PASSWORD, &greenBlinker, &redBlinker);
}

void loop()
{
    bool coldInc = processBouncer(coldBouncer);
    bool hotInc = processBouncer(hotBouncer);

    if (coldInc || hotInc)
        DataStorage::incrementCounters(coldInc, hotInc);

    DataStorage::work();

    greenBlinker.work();
    redBlinker.work();

    // Debug routines
    processConsoleInput(&greenBlinker, &redBlinker);
    // testSend();
    // delay(50);
}
