#ifndef JJR_PULSER_STORAGE_H
#define JJR_PULSER_STORAGE_H

#include <string>
#include <ESP8266WiFi.h>

#include "blinker.h"

class DataStorage {
public:
    static DataStorage & Instance();
    void setup(const char *s, const char *p, Blinker *gb, Blinker *rb);

private:
    DataStorage() { }
    ~DataStorage() { }

    DataStorage(DataStorage const &);
    DataStorage & operator=(DataStorage const &);

    static void onConnected(const WiFiEventStationModeConnected &e);
    static void onDisconnected(const WiFiEventStationModeDisconnected &e);
    static void onAuthModeChanged(const WiFiEventStationModeAuthModeChanged &e);
    static void onGotIP(const WiFiEventStationModeGotIP &e);
    static void onDHCPTimeout(void);

    static const char * printDisconnectReason(WiFiDisconnectReason r);

    static Blinker *m_greenLed;
    static Blinker *m_redLed;

    std::string m_wifiSSID;
    std::string m_wifiPassword;

    WiFiEventHandler m_onConnectedHandler;
    WiFiEventHandler m_onDisconnectedHandler;
    WiFiEventHandler m_onAuthModeChangedHandler;
    WiFiEventHandler m_onGotIPHandler;
    WiFiEventHandler m_onDHCPTimeoutHandler;
};

#endif // JJR_PULSER_STORAGE_H
