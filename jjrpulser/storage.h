#ifndef JJR_PULSER_STORAGE_H
#define JJR_PULSER_STORAGE_H

#include <vector>

#include <ESP8266WiFi.h>
#include <asyncHTTPrequest.h>

#include "blinker.h"

#define WATER_COLD_INCREMENT 10
#define WATER_HOT_INCREMENT 10

class DataStorage {
  public:
    static DataStorage & Instance();
    void setup(const char *s, const char *p, Blinker *gb, Blinker *rb);
    void work();
    void incrementCounters(bool cold, bool hot);

  private:
    DataStorage();
    ~DataStorage();

    DataStorage(DataStorage const &);
    DataStorage & operator=(DataStorage const &);

    void httpStateChanged(asyncHTTPrequest *, int readyState);

    static void onConnected(const WiFiEventStationModeConnected &e);
    static void onDisconnected(const WiFiEventStationModeDisconnected &e);
    static void onAuthModeChanged(const WiFiEventStationModeAuthModeChanged &e);
    static void onGotIP(const WiFiEventStationModeGotIP &e);
    static void onDHCPTimeout(void);

    static const char * printDisconnectReason(WiFiDisconnectReason r);

    static void globalHTTPStateChanged(void *, asyncHTTPrequest *, int readyState);

    static Blinker *m_greenLed;
    static Blinker *m_redLed;

    String m_wifiSSID;
    String m_wifiPassword;

    uint32_t m_coldWaterCouner;
    uint32_t m_hotWaterCouner;

    WiFiEventHandler m_onConnectedHandler;
    WiFiEventHandler m_onDisconnectedHandler;
    WiFiEventHandler m_onAuthModeChangedHandler;
    WiFiEventHandler m_onGotIPHandler;
    WiFiEventHandler m_onDHCPTimeoutHandler;

    std::vector<std::shared_ptr<asyncHTTPrequest> > m_httpRequestsList;
};

#endif // JJR_PULSER_STORAGE_H
