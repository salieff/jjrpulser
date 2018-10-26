#include "bouncer.h"
#include "blinker.h"

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
    }
#undef SWITCHLED
}

void processBouncer(PinBouncer &pb)
{
    pb.work();

    if (!pb.stable())
        return;

    if (!pb.newValue())
        return;

    Serial.printf("%s: %d\n", pb.name().c_str(), pb.value());
    pb.resetNewValue();
}

void setup()
{
    Serial.begin(74880);
    Serial.flush();
    Serial.printf("\r\nJJR Pulser Setup\r\n");
    Serial.flush();

    coldBouncer.setup();
    hotBouncer.setup();

    greenBlinker.setup();
    redBlinker.setup();

    greenBlinker.setMode(Blinker::Data);
    redBlinker.setMode(Blinker::Setup);
}

void loop()
{
    processConsoleInput();

    greenBlinker.work();
    redBlinker.work();

    processBouncer(coldBouncer);
    processBouncer(hotBouncer);

    // delay(50);
}
