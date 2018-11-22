#include "bouncer.h"
#include "blinker.h"
#include "storage.h"
#include "passwords.h"

PinBouncer coldBouncer(COLD_PIN_NUMBER, "Cold");
PinBouncer hotBouncer(HOT_PIN_NUMBER, "Hot");

Blinker greenBlinker(GREEN_LED_PIN_NUMBER);
Blinker redBlinker(RED_LED_PIN_NUMBER);

unsigned long upTimeDays = 0;
unsigned long upTimeHours = 0;
unsigned long upTimeMinutes = 0;
unsigned long upTimeSeconds = 0;
unsigned long upTimeMillis = 0;
unsigned long lastMillis = 0;

uint32_t lastFreeHeap = 0;

unsigned long lastTestSendMillis = 0;

void checkUptime()
{
    unsigned long timestamp = millis();
    unsigned long delta = timestamp - lastMillis;
    lastMillis = timestamp;

    upTimeMillis += delta;

    upTimeSeconds += upTimeMillis / 1000ul;
    upTimeMillis %= 1000ul;

    upTimeMinutes += upTimeSeconds / 60ul;
    upTimeSeconds %= 60ul;

    upTimeHours += upTimeMinutes / 60ul;
    upTimeMinutes %= 60ul;

    upTimeDays += upTimeHours / 24ul;
    upTimeHours %= 24ul;
}

void printFreeHeap()
{
    uint32_t memcurr = ESP.getFreeHeap();
    Serial.printf("FREEHeap: %d; DIFF %d\n", memcurr, memcurr - lastFreeHeap);
    lastFreeHeap = memcurr;
}

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
        DataStorage::incrementCounters(true, false);
        break;

    case 'h':
        Serial.println("Emulate HOT signal");
        DataStorage::incrementCounters(false, true);
        break;

    case 'u':
        Serial.printf("Uptime: %lu days %02lu:%02lu:%02lu\r\n", upTimeDays, upTimeHours, upTimeMinutes, upTimeSeconds);
        DataStorage::printStatistics();
        break;

    case 'm':
        printFreeHeap();
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

    lastFreeHeap = ESP.getFreeHeap();
    lastTestSendMillis = millis();
}

void testSend()
{
    unsigned long timestamp = millis();
    unsigned long delta = timestamp - lastTestSendMillis;
    if (delta < 15000)
        return;

    lastTestSendMillis = timestamp;
    printFreeHeap();

    DataStorage::incrementCounters(true, false);
    DataStorage::incrementCounters(false, true);
    DataStorage::incrementCounters(true, true);
}

void loop()
{
    checkUptime();
    processConsoleInput();

    DataStorage::work();

    greenBlinker.work();
    redBlinker.work();

    bool coldInc = processBouncer(coldBouncer);
    bool hotInc = processBouncer(hotBouncer);

    if (coldInc || hotInc)
        DataStorage::incrementCounters(coldInc, hotInc);

    testSend();
    // delay(50);
}
