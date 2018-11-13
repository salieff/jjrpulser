#ifndef JJR_PULSER_BLINKER_H
#define JJR_PULSER_BLINKER_H

#include <Arduino.h>

#define GREEN_LED_PIN_NUMBER 12
#define RED_LED_PIN_NUMBER 13

class Blinker {
public:
    enum Mode {
        Off,
        Work,
        Data,
        Setup,
        Error,
        MaxMode
    };

    Blinker(uint8_t p);

    void setup();
    void work();
    Mode mode() const;
    void setMode(Mode m);

private:
    struct BlinkTimes {
        unsigned long lowTime;
        unsigned long highTime;
        unsigned long fullTime;
        Mode nextMode;
    };

    static const BlinkTimes m_blinkTimers[MaxMode];

    uint8_t m_pinNumber;
    unsigned long m_lastTimestamp;
    unsigned long m_fullTimestamp;
    uint8_t m_subMode;
    Mode m_currentMode;
};

#endif // JJR_PULSER_BLINKER_H
