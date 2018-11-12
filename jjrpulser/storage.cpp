#include "storage.h"

Blinker * DataStorage::m_greenLed = NULL;
Blinker * DataStorage::m_redLed = NULL;

DataStorage::DataStorage()
    : m_coldWaterCouner(0)
    , m_hotWaterCouner(0)
{
}

DataStorage::~DataStorage()
{
}

DataStorage & DataStorage::Instance()
{
    static DataStorage d;
    return d;
}

void DataStorage::setup(const char *s, const char *p, Blinker *gb, Blinker *rb)
{
    m_wifiSSID = s;
    m_wifiPassword = p;

    m_greenLed = gb;
    m_redLed = rb;

    m_onConnectedHandler = WiFi.onStationModeConnected(DataStorage::onConnected);
    m_onDisconnectedHandler = WiFi.onStationModeDisconnected(DataStorage::onDisconnected);
    m_onAuthModeChangedHandler = WiFi.onStationModeAuthModeChanged(DataStorage::onAuthModeChanged);
    m_onGotIPHandler = WiFi.onStationModeGotIP(DataStorage::onGotIP);
    m_onDHCPTimeoutHandler = WiFi.onStationModeDHCPTimeout(DataStorage::onDHCPTimeout);

    Serial.printf("[WIFI] Attempting to connect to %s\r\n", m_wifiSSID.c_str());
    WiFi.begin(m_wifiSSID.c_str(), m_wifiPassword.c_str());

    m_greenLed->setMode(Blinker::Data);
    m_redLed->setMode(Blinker::Off);
}

void DataStorage::work()
{
    auto it = m_httpRequestsList.begin();
    while(it != m_httpRequestsList.end())
        if ((*it)->readyState() == asyncHTTPrequest::readyStateDone)
            m_httpRequestsList.erase(it);
        else
            ++it;
}

void DataStorage::incrementCounters(bool cold, bool hot)
{
    if (cold)
        m_coldWaterCouner += WATER_COLD_INCREMENT;

    if (hot)
        m_hotWaterCouner += WATER_HOT_INCREMENT;

    Serial.printf("[DataStorage::incrementCounters %lu] Attempting to send HTTP data", millis());
    
    if (cold)
        Serial.printf(" COLD = %u", m_coldWaterCouner);

    if (hot)
        Serial.printf(" HOT = %u", m_hotWaterCouner);

    Serial.println();

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("[DataStorage::incrementCounters] No Wi-Fi connections available, can't send data");
        return;
    }

    std::shared_ptr<asyncHTTPrequest> ahr(new asyncHTTPrequest);
    ahr->setTimeout(180);
    ahr->onReadyStateChange(DataStorage::globalHTTPStateChanged, this);

    m_httpRequestsList.push_back(ahr);
    Serial.printf("[DataStorage::incrementCounters] requests list size %u\r\n", m_httpRequestsList.size());

    String url = "http://salieff.phantomazz.me:5190/jjrpulser/text.sh?cmd=add_value";

    if (cold)
    {
        url += "&cold=";
        url += String(m_coldWaterCouner);
    }

    if (hot)
    {
        url += "&hot=";
        url += String(m_hotWaterCouner);
    }

    if (!ahr->open(asyncHTTPrequest::HTTPmethodGET, url.c_str()))
        Serial.printf("[DataStorage::incrementCounters %lu] Error while opening HTTP request\r\n", millis());
    else
        ahr->send();
}

void DataStorage::onConnected(const WiFiEventStationModeConnected &e)
{
    Serial.printf("[WIFI %lu] Connected to %s %02X:%02X:%02X:%02X:%02X:%02X ch %u\r\n", millis(), e.ssid.c_str(), e.bssid[0], e.bssid[1], e.bssid[2], e.bssid[3], e.bssid[4], e.bssid[5], e.channel);

    m_greenLed->setMode(Blinker::Data);
    m_redLed->setMode(Blinker::Off);
}

void DataStorage::onDisconnected(const WiFiEventStationModeDisconnected &e)
{
    Serial.printf("[WIFI %lu] Disconnected from %s %02X:%02X:%02X:%02X:%02X:%02X reason %s\r\n", millis(), e.ssid.c_str(), e.bssid[0], e.bssid[1], e.bssid[2], e.bssid[3], e.bssid[4], e.bssid[5], printDisconnectReason(e.reason));

    m_greenLed->setMode(Blinker::Work);
    m_redLed->setMode(Blinker::Error);
}

void DataStorage::onAuthModeChanged(const WiFiEventStationModeAuthModeChanged &e)
{
    Serial.printf("[WIFI %lu] Auth mode changed from %u to %u\r\n", millis(), e.oldMode, e.newMode);
}

void DataStorage::onGotIP(const WiFiEventStationModeGotIP &e)
{
    Serial.printf("[WIFI %lu] Got IP ", millis());
    Serial.println(e.ip);

    Serial.print("    Mask    ");
    Serial.println(e.mask);

    Serial.print("    Gateway ");
    Serial.println(e.gw);

    m_greenLed->setMode(Blinker::Work);
    m_redLed->setMode(Blinker::Off);
}

void DataStorage::onDHCPTimeout(void)
{
    Serial.printf("[WIFI %lu] DHCP timeout\r\n", millis());

    m_greenLed->setMode(Blinker::Work);
    m_redLed->setMode(Blinker::Error);
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

void DataStorage::httpStateChanged(asyncHTTPrequest *r, int readyState)
{
    Serial.printf("[DataStorage::onHTTPStateChanged %lu] HTTP GET readyState: %d\r\n", millis(), readyState);

    if (readyState != asyncHTTPrequest::readyStateDone)
        return;

    Serial.printf("[DataStorage::onHTTPStateChanged] HTTP Code: %d\r\n", r->responseHTTPcode());
    if (r->responseHTTPcode() < 0)
        return;

    Serial.println(r->responseText());
}

void DataStorage::globalHTTPStateChanged(void *arg, asyncHTTPrequest *r, int readyState)
{
    DataStorage *ds = static_cast<DataStorage *>(arg);
    ds->httpStateChanged(r, readyState);
}
