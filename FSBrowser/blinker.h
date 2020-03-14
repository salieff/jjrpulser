#ifndef JJR_PULSER_BLINKER_H
#define JJR_PULSER_BLINKER_H

#include <Arduino.h>
#include <functional>

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

    typedef std::function<void(Blinker &, Mode, Mode, void *)> TCallbackFunction;

    Blinker(uint8_t p, const char *n = nullptr, TCallbackFunction f = m_emptyFunction, void *ua = nullptr);

    void setup();
    void work();
    Mode mode() const;
    String modeName() const;
    void setMode(Mode m, bool suppressCallback = false);
    void setMode(String m, bool suppressCallback = false);
    const String & name() const;

private:
    struct BlinkTimes {
        unsigned long lowTime;
        unsigned long highTime;
        unsigned long fullTime;
        Mode nextMode;
    };

    static const BlinkTimes m_blinkTimers[MaxMode];

    uint8_t m_pinNumber;
    String m_pinName;
    unsigned long m_lastTimestamp;
    unsigned long m_fullTimestamp;
    uint8_t m_subMode;
    Mode m_currentMode;

    static TCallbackFunction m_emptyFunction;
    TCallbackFunction m_callbackFunction;
    void *m_callbackUserArgument;

    static Mode string2Mode(String m);
    static String mode2String(Mode m);
};

#endif // JJR_PULSER_BLINKER_H
