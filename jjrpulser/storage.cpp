#include "storage.h"

DataStorage & DataStorage::Instance()
{
    static DataStorage d;
    return d;
}

void DataStorage::setup(const char *s, const char *p)
{
    m_wifiSSID = s;
    m_wifiPassword = p;

    WiFi.onStationModeConnected(DataStorage::onConnected);
    WiFi.onStationModeDisconnected(DataStorage::onDisconnected);
    WiFi.onStationModeAuthModeChanged(DataStorage::onAuthModeChanged);
    WiFi.onStationModeGotIP(DataStorage::onGotIP);
    WiFi.onStationModeDHCPTimeout(DataStorage::onDHCPTimeout);

    WiFi.begin(m_wifiSSID.c_str(), m_wifiPassword.c_str());
}

void DataStorage::onConnected(const WiFiEventStationModeConnected &e)
{
    Serial.printf("[WIFI] Connected to %s %hhu:%hhu:%hhu:%hhu:%hhu:%hhu ch %hhu\r\n", e.ssid.c_str(), e.bssid[0], e.bssid[1], e.bssid[2], e.bssid[3], e.bssid[4], e.bssid[5], e.channel);
}

void DataStorage::onDisconnected(const WiFiEventStationModeDisconnected &e)
{
    Serial.printf("[WIFI] Disconnected from %s %hhu:%hhu:%hhu:%hhu:%hhu:%hhu reason %s\r\n", e.ssid.c_str(), e.bssid[0], e.bssid[1], e.bssid[2], e.bssid[3], e.bssid[4], e.bssid[5], printDisconnectReason(e.reason));
}

void DataStorage::onAuthModeChanged(const WiFiEventStationModeAuthModeChanged &e)
{
}

void DataStorage::onGotIP(const WiFiEventStationModeGotIP &e)
{
}

void DataStorage::onDHCPTimeout(void)
{
}

const char * DataStorage::printDisconnectReason(WiFiDisconnectReason r)
{
#define PRINT_DISCONNECT_REASON(arg) if (r == WIFI_DISCONNECT_REASON_##arg) return #arg;
    PRINT_DISCONNECT_REASON(UNSPECIFIED)
    PRINT_DISCONNECT_REASON(AUTH_EXPIRE)
    PRINT_DISCONNECT_REASON(AUTH_LEAVE)
    PRINT_DISCONNECT_REASON(ASSOC_EXPIRE)
    PRINT_DISCONNECT_REASON(ASSOC_TOOMANY)
    PRINT_DISCONNECT_REASON(NOT_AUTHED)
    PRINT_DISCONNECT_REASON(NOT_ASSOCED)
    PRINT_DISCONNECT_REASON(ASSOC_LEAVE)
    PRINT_DISCONNECT_REASON(ASSOC_NOT_AUTHED)
    PRINT_DISCONNECT_REASON(DISASSOC_PWRCAP_BAD)
    PRINT_DISCONNECT_REASON(DISASSOC_SUPCHAN_BAD)
    PRINT_DISCONNECT_REASON(IE_INVALID)
    PRINT_DISCONNECT_REASON(MIC_FAILURE)
    PRINT_DISCONNECT_REASON(4WAY_HANDSHAKE_TIMEOUT)
    PRINT_DISCONNECT_REASON(GROUP_KEY_UPDATE_TIMEOUT)
    PRINT_DISCONNECT_REASON(IE_IN_4WAY_DIFFERS)
    PRINT_DISCONNECT_REASON(GROUP_CIPHER_INVALID)
    PRINT_DISCONNECT_REASON(PAIRWISE_CIPHER_INVALID)
    PRINT_DISCONNECT_REASON(AKMP_INVALID)
    PRINT_DISCONNECT_REASON(UNSUPP_RSN_IE_VERSION)
    PRINT_DISCONNECT_REASON(INVALID_RSN_IE_CAP)
    PRINT_DISCONNECT_REASON(802_1X_AUTH_FAILED)
    PRINT_DISCONNECT_REASON(CIPHER_SUITE_REJECTED)
    PRINT_DISCONNECT_REASON(BEACON_TIMEOUT)
    PRINT_DISCONNECT_REASON(NO_AP_FOUND)
    PRINT_DISCONNECT_REASON(AUTH_FAIL)
    PRINT_DISCONNECT_REASON(ASSOC_FAIL)
    PRINT_DISCONNECT_REASON(HANDSHAKE_TIMEOUT)
#undef PRINT_DISCONNECT_REASON

    return "Unresolved disconnect reason";
}
