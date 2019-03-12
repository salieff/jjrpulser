#include "test.h"
#include "storage.h"
#include "blinker.h"

unsigned long lastTestSendMillis = 0;

void processConsoleInput(Blinker *greenBlinker, Blinker *redBlinker)
{
    if (Serial.available() <= 0)
        return;

#define SWITCHLED(color, mode) \
      Serial.printf("Switch "#color" led to "#mode" mode\r\n"); \
      color##Blinker->setMode(Blinker::mode); \
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
        DataStorage::printStatistics();
        break;
    }
#undef SWITCHLED
}

void testSend()
{
    unsigned long timestamp = millis();
    unsigned long delta = timestamp - lastTestSendMillis;
    if (delta < TEST_SEND_PERIOD)
        return;

    lastTestSendMillis = timestamp;

    DataStorage::printStatistics();

    DataStorage::incrementCounters(true, false);
    DataStorage::incrementCounters(false, true);
    DataStorage::incrementCounters(true, true);
}
