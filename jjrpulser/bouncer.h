#ifndef JJR_PULSER_BOUNCER_H
#define JJR_PULSER_BOUNCER_H

#include <Arduino.h>

#define COLD_PIN_NUMBER 4
#define HOT_PIN_NUMBER 5

class PinBouncer {
public:
    PinBouncer(uint8_t p, const char *n, unsigned long t = 500);

    void setup();
    void work();
    bool stable() const;
    bool newValue() const;
    void resetNewValue();
    int value() const;
    const char * name() const;

private:
    uint8_t m_pinNumber;
    String m_pinName;
    unsigned long m_timeout;
    int m_lastPinState;
    int m_lastStableState;
    unsigned long m_lastTimestamp;
    bool m_newValue;
    bool m_isStable;
};

#endif // JJR_PULSER_BOUNCER_H
