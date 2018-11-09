#include "bouncer.h"
#include "blinker.h"
#include "storage.h"
#include "passwords.h"

PinBouncer coldBouncer(COLD_PIN_NUMBER, "Cold");
PinBouncer hotBouncer(HOT_PIN_NUMBER, "Hot");

Blinker greenBlinker(GREEN_LED_PIN_NUMBER);
Blinker redBlinker(RED_LED_PIN_NUMBER);

void processConsoleInput()
{
    if (Serial.available() <= 0)
        return;

#define SWITCHLED(color, mode) \
      Serial.printf("Switch "#color" led to "#mode" mode\r\n"); \
      color##Blinker.setMode(Blinker::mode); \
      break;

    int byte = Serial.read();
    switch(byte)
    {
    case '0':
        SWITCHLED(green, Off)

    case '1':
        SWITCHLED(green, Work)

    case '2':
        SWITCHLED(green, Data)

    case '3':
        SWITCHLED(green, Setup)

    case '4':
        SWITCHLED(green, Error)

    case 'g':
        SWITCHLED(red, Off)

    case 'a':
        SWITCHLED(red, Work)

    case 's':
        SWITCHLED(red, Data)

    case 'd':
        SWITCHLED(red, Setup)

    case 'f':
        SWITCHLED(red, Error)

    case 'c':
        Serial.println("Emulate COLD signal");
        DataStorage::Instance().incrementCounters(true, false);
        break;

    case 'h':
        Serial.println("Emulate HOT signal");
        DataStorage::Instance().incrementCounters(false, true);
        break;
    }
#undef SWITCHLED
}

bool processBouncer(PinBouncer &pb)
{
    pb.work();

    if (!pb.stable())
        return false;

    if (!pb.newValue())
        return false;

    Serial.printf("%s: %d\n", pb.name().c_str(), pb.value());
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

    DataStorage::Instance().setup(JJR_PULSER_WIFI_SSID, JJR_PULSER_WIFI_PASSWORD, &greenBlinker, &redBlinker);
}

void loop()
{
    processConsoleInput();

    greenBlinker.work();
    redBlinker.work();

    bool coldInc = processBouncer(coldBouncer);
    bool hotInc = processBouncer(hotBouncer);

    if (coldInc || hotInc)
        DataStorage::Instance().incrementCounters(coldInc, hotInc);

    // delay(50);
}
