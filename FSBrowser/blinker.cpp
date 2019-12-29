#include "blinker.h"

const Blinker::BlinkTimes Blinker::m_blinkTimers[Blinker::MaxMode] = {
    {0, 0, 0, Blinker::Off}, // Off
    {3000, 30, 0, Blinker::Off}, // Work
    {100, 30,  5000, Blinker::Work}, // Data
    {100, 1000, 5000, Blinker::Work}, // Setup
    {500, 500, 0, Blinker::Off}  // Error
};

Blinker::Blinker(uint8_t p, const char *n, TCallbackFunction f)
    : m_pinNumber(p)
    , m_pinName(n)
    , m_callbackFunction(f)
    , m_lastTimestamp(0)
    , m_fullTimestamp(0)
    , m_subMode(LOW)
    , m_currentMode(Off)
{
}

void Blinker::setup()
{
    pinMode(m_pinNumber, OUTPUT);
    m_lastTimestamp = m_fullTimestamp = millis();
}

void Blinker::work()
{
    unsigned long timestamp = millis();

    const Blinker::BlinkTimes &bt = m_blinkTimers[m_currentMode];
    if (bt.fullTime > 0 && timestamp - m_fullTimestamp >= bt.fullTime)
    {
        setMode(bt.nextMode);
        return;
    }

    if (bt.lowTime > 0 && m_subMode == LOW && timestamp - m_lastTimestamp >= bt.lowTime)
    {
        m_lastTimestamp = timestamp;
        m_subMode = HIGH;
        digitalWrite(m_pinNumber, HIGH);
        return;
    }

    if (bt.highTime > 0 && m_subMode == HIGH && timestamp - m_lastTimestamp >= bt.highTime)
    {
        m_lastTimestamp = timestamp;
        m_subMode = LOW;
        digitalWrite(m_pinNumber, LOW);
        return;
    }
}

Blinker::Mode Blinker::mode() const
{
    return m_currentMode;
}

String Blinker::modeName() const
{
    return mode2String(m_currentMode);
}

void Blinker::setMode(Mode m)
{
    if (m_currentMode == m)
        return;

    m_fullTimestamp = m_lastTimestamp = millis();
    m_subMode = LOW;
    digitalWrite(m_pinNumber, LOW);

    if (m_callbackFunction)
        m_callbackFunction(*this, m_currentMode, m);

    m_currentMode = m;
}

void Blinker::setMode(String m)
{
    setMode(Blinker::string2Mode(m));
}

Mode Blinker::string2Mode(String m)
{
    if (m == "off")
        return Blinker::Off;

    if (m == "work")
        return Blinker::Work;

    if (m == "data")
        return Blinker::Data;

    if (m == "setup")
        return Blinker::Setup;

    return Blinker::Error;
}

String Blinker::mode2String(Mode m)
{
    switch(m)
    {
        case Blinker::Off :
            return "off";

        case Blinker::Work :
            return "work";

        case Blinker::Data :
            return "data";

        case Blinker::Setup :
            return "setup";

        case Blinker::Error :
            return "error";
    }

    return "error";
}
