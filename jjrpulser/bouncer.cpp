#include "bouncer.h"

PinBouncer::PinBouncer(uint8_t p, const char *n, unsigned long t)
    : m_pinNumber(p)
    , m_pinName(n)
    , m_timeout(t)
    , m_lastPinState(HIGH)
    , m_lastStableState(HIGH)
    , m_lastTimestamp(0)
    , m_newValue(false)
    , m_isStable(false)
{
}

void PinBouncer::setup()
{
    pinMode(m_pinNumber, INPUT);
    m_lastTimestamp = millis();
    m_lastStableState = m_lastPinState = digitalRead(m_pinNumber);
}

void PinBouncer::work()
{
    unsigned long timestamp = millis();
    int pinState = digitalRead(m_pinNumber);

    if (pinState != m_lastPinState)
    {
        m_lastPinState = pinState;
        m_lastTimestamp = timestamp;
        m_isStable = false;

        return;
    }

    if (timestamp - m_lastTimestamp < m_timeout)
        return;

    m_isStable = true;

    if (pinState == m_lastStableState)
        return;

    m_lastStableState = pinState;
    m_newValue = true;
}

bool PinBouncer::stable() const
{
    return m_isStable;
}

bool PinBouncer::newValue() const
{
    return m_newValue;
}

void PinBouncer::resetNewValue()
{
    m_newValue = false;
}

int PinBouncer::value() const
{
    return m_lastPinState;
}

const char * PinBouncer::name() const
{
    return m_pinName.c_str();
}
