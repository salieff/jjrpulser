#ifndef JJR_PULSER_STORAGE_H
#define JJR_PULSER_STORAGE_H

#include <string>
#include <ESP8266WiFi.h>
#include <asyncHTTPrequest.h>

#include "blinker.h"

class DataStorage {
public:
    static DataStorage & Instance();
    void setup(const char *s, const char *p, Blinker *gb, Blinker *rb);
    void incrementCounters(bool cold, bool hot);

private:
    DataStorage();
    ~DataStorage();

    DataStorage(DataStorage const &);
    DataStorage & operator=(DataStorage const &);

    static void onConnected(const WiFiEventStationModeConnected &e);
    static void onDisconnected(const WiFiEventStationModeDisconnected &e);
    static void onAuthModeChanged(const WiFiEventStationModeAuthModeChanged &e);
    static void onGotIP(const WiFiEventStationModeGotIP &e);
    static void onDHCPTimeout(void);

    static const char * printDisconnectReason(WiFiDisconnectReason r);

    static void onHTTPStateChanged(void *, asyncHTTPrequest *, int readyState);

    static Blinker *m_greenLed;
    static Blinker *m_redLed;

    std::string m_wifiSSID;
    std::string m_wifiPassword;

    uint32_t m_coldWaterCouner;
    uint32_t m_hotWaterCouner;

    WiFiEventHandler m_onConnectedHandler;
    WiFiEventHandler m_onDisconnectedHandler;
    WiFiEventHandler m_onAuthModeChangedHandler;
    WiFiEventHandler m_onGotIPHandler;
    WiFiEventHandler m_onDHCPTimeoutHandler;

    asyncHTTPrequest m_httpRequest;
};

#endif // JJR_PULSER_STORAGE_H
