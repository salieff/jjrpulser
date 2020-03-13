#include "bouncer.h"
#include "blinker.h"

PinBouncer coldBouncer(COLD_PIN_NUMBER, "Cold", 100);
PinBouncer hotBouncer(HOT_PIN_NUMBER, "Hot", 100);

Blinker greenBlinker(GREEN_LED_PIN_NUMBER);
Blinker redBlinker(RED_LED_PIN_NUMBER);

struct {
    unsigned long m_lastMillis;

    unsigned long m_upTimeDays;
    unsigned long m_upTimeHours;
    unsigned long m_upTimeMinutes;
    unsigned long m_upTimeSeconds;
    unsigned long m_upTimeMillis;

    uint32_t m_freeHeap;
} m_statistics = {0, 0, 0, 0, 0, 0, 0};


void refreshStatistics(unsigned long delta = 0)
{
    if (delta == 0)
    {
        unsigned long timestamp = millis();
        delta = timestamp - m_statistics.m_lastMillis;
        m_statistics.m_lastMillis = timestamp;
    }

    if (delta == 0)
        return;

    m_statistics.m_upTimeMillis += delta;

    m_statistics.m_upTimeSeconds += m_statistics.m_upTimeMillis / 1000ul;
    m_statistics.m_upTimeMillis %= 1000ul;

    m_statistics.m_upTimeMinutes += m_statistics.m_upTimeSeconds / 60ul;
    m_statistics.m_upTimeSeconds %= 60ul;

    m_statistics.m_upTimeHours += m_statistics.m_upTimeMinutes / 60ul;
    m_statistics.m_upTimeMinutes %= 60ul;

    m_statistics.m_upTimeDays += m_statistics.m_upTimeHours / 24ul;
    m_statistics.m_upTimeHours %= 24ul;

    m_statistics.m_freeHeap = ESP.getFreeHeap();
}

void printStatistics()
{
    uint32_t lastFreeHeap = m_statistics.m_freeHeap;

    refreshStatistics();

    Serial.printf("Uptime: %lu days %02lu:%02lu:%02lu\r\n", m_statistics.m_upTimeDays, m_statistics.m_upTimeHours, m_statistics.m_upTimeMinutes, m_statistics.m_upTimeSeconds);
    Serial.printf("FREEHeap: %u; DIFF %d\n", m_statistics.m_freeHeap, m_statistics.m_freeHeap - lastFreeHeap);
}

void processBlinkerAndBouncer(PinBouncer &pb, Blinker &bl)
{
    bl.work();
    pb.work();

    if (!pb.stable())
        return;

    if (!pb.newValue())
        return;

    Serial.printf("%s: %d\n", pb.name(), pb.value());
    pb.resetNewValue();

    if (pb.value() == LOW)
        bl.setMode(Blinker::Data);
    else
        bl.setMode(Blinker::Work);
}

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

    case 'u':
        printStatistics();
        break;
    }
#undef SWITCHLED
}

void setup()
{
    Serial.begin(115200);
    Serial.flush();
    Serial.printf("\r\nJJR Pulser Setup\r\n");
    Serial.flush();

    coldBouncer.setup();
    hotBouncer.setup();

    greenBlinker.setup();
    redBlinker.setup();

    greenBlinker.setMode(Blinker::Error);
    redBlinker.setMode(Blinker::Error);
}

void loop()
{
    processBlinkerAndBouncer(coldBouncer, greenBlinker);
    processBlinkerAndBouncer(hotBouncer, redBlinker);

    processConsoleInput(&greenBlinker, &redBlinker);

    refreshStatistics();
}
