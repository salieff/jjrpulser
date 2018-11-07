#ifndef JJR_PULSER_STORAGE_H
#define JJR_PULSER_STORAGE_H

#include <string>
#include <ESP8266WiFi.h>

class DataStorage {
public:
    static DataStorage & Instance();
    void setup(const char *s, const char *p);

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

    std::string m_wifiSSID;
    std::string m_wifiPassword;
};

#endif // JJR_PULSER_STORAGE_H
