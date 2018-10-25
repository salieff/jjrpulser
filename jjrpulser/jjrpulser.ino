#include "bouncer.h"
#include "blinker.h"

PinBouncer coldBouncer(COLD_PIN_NUMBER, "Cold");
PinBouncer hotBouncer(HOT_PIN_NUMBER, "Hot");

Blinker greenBlinker(GREEN_LED_PIN_NUMBER);
Blinker redBlinker(RED_LED_PIN_NUMBER);

void processBouncer(PinBouncer &pb, Blinker &bl)
{
    pb.work();
    bl.work();

    if (!pb.stable())
        return;

    if (!pb.newValue())
        return;

    if (pb.value() == LOW)
    {
        Blinker::Mode m = (Blinker::Mode)((int)bl.mode() + 1);
        if (m == Blinker::MaxMode)
            m = Blinker::Off;

        bl.setMode(m);
    }

    Serial.printf("%s: %d\n", pb.name().c_str(), pb.value());
    pb.resetNewValue();
}

void setup()
{
    Serial.begin(115200);
    Serial.println("JJR Pulser Setup");

    coldBouncer.setup();
    hotBouncer.setup();

    greenBlinker.setup();
    redBlinker.setup();

    greenBlinker.setMode(Blinker::Data);
    redBlinker.setMode(Blinker::Setup);
}

void loop()
{
    processBouncer(coldBouncer, greenBlinker);
    processBouncer(hotBouncer, redBlinker);

    // delay(50);
}
