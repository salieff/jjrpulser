#include "storage.h"
#include "httprequest.h"

namespace DataStorage {

Blinker *m_greenLed = NULL;
Blinker *m_redLed = NULL;

uint32_t m_coldWaterCounter = 0;
uint32_t m_hotWaterCounter = 0;

WiFiEventHandler m_onConnectedHandler = NULL;
WiFiEventHandler m_onDisconnectedHandler = NULL;
WiFiEventHandler m_onAuthModeChangedHandler = NULL;
WiFiEventHandler m_onGotIPHandler = NULL;
WiFiEventHandler m_onDHCPTimeoutHandler = NULL;

uint32_t m_httpErrorsCounter = 0;

uint32_t m_httpRequestsListSize = 0;
// asyncHTTPrequest *m_httpRequestsList[HTTP_CONN_LIST_MAX];
LWIP_HTTPRequest *m_httpRequestsList[HTTP_CONN_LIST_MAX];

String m_macAddress;

// -----=====+++++oooooOOOOO WIFI Callbacks OOOOOooooo+++++=====-----
/*
const char * printDisconnectReason(WiFiDisconnectReason r)
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
*/

void onConnected(const WiFiEventStationModeConnected &e)
{
    Serial.printf("[WIFI %lu] Connected to %s %02X:%02X:%02X:%02X:%02X:%02X ch %u\r\n", millis(), e.ssid.c_str(), e.bssid[0], e.bssid[1], e.bssid[2], e.bssid[3], e.bssid[4], e.bssid[5], e.channel);

    m_greenLed->setMode(Blinker::Data);
    m_redLed->setMode(Blinker::Off);
}

void onDisconnected(const WiFiEventStationModeDisconnected &e)
{
    // Serial.printf("[WIFI %lu] Disconnected from %s %02X:%02X:%02X:%02X:%02X:%02X reason %s\r\n", millis(), e.ssid.c_str(), e.bssid[0], e.bssid[1], e.bssid[2], e.bssid[3], e.bssid[4], e.bssid[5], printDisconnectReason(e.reason));
    Serial.printf("[WIFI %lu] Disconnected from %s %02X:%02X:%02X:%02X:%02X:%02X reason %d\r\n", millis(), e.ssid.c_str(), e.bssid[0], e.bssid[1], e.bssid[2], e.bssid[3], e.bssid[4], e.bssid[5], e.reason);

    m_greenLed->setMode(Blinker::Work);
    m_redLed->setMode(Blinker::Error);
}

void onAuthModeChanged(const WiFiEventStationModeAuthModeChanged &e)
{
    Serial.printf("[WIFI %lu] Auth mode changed from %u to %u\r\n", millis(), e.oldMode, e.newMode);
}

void onGotIP(const WiFiEventStationModeGotIP &e)
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

void onDHCPTimeout(void)
{
    Serial.printf("[WIFI %lu] DHCP timeout\r\n", millis());

    m_greenLed->setMode(Blinker::Work);
    m_redLed->setMode(Blinker::Error);
}
// -----=====+++++oooooOOOOO End of WIFI Callbacks OOOOOooooo+++++=====-----

// -----=====+++++oooooOOOOO HTTP requests list functions OOOOOooooo+++++=====-----
/*
void httpStateChanged(void *, asyncHTTPrequest *r, int readyState)
{
    Serial.printf("[DataStorage::onHTTPStateChanged %lu] HTTP GET readyState: %d\r\n", millis(), readyState);

    if (readyState != asyncHTTPrequest::readyStateDone)
        return;

    Serial.printf("[DataStorage::onHTTPStateChanged] HTTP Code: %d\r\n", r->responseHTTPcode());
    if (r->responseHTTPcode() < 0)
    {
        ++m_httpErrorsCounter;
        return;
    }

    Serial.println(r->responseText());
}
*/

// bool addToHttpRequestsList(String &url)
bool addToHttpRequestsList(const char *host, uint16_t port, String &url)
{
    if (m_httpRequestsListSize >= HTTP_CONN_LIST_MAX)
    {
        Serial.printf("[DataStorage::addToHttpRequestsList] List max size %d reached\r\n", HTTP_CONN_LIST_MAX);
        return false;
    }

    /*
    asyncHTTPrequest *ahr = new asyncHTTPrequest;
    ahr->setTimeout(180);
    ahr->onReadyStateChange(httpStateChanged);
    */

    LWIP_HTTPRequest *ahr = new LWIP_HTTPRequest(host, port, url.c_str());

    m_httpRequestsList[m_httpRequestsListSize] = ahr;
    ++m_httpRequestsListSize;

    /*
    if (!ahr->open(asyncHTTPrequest::HTTPmethodGET, url.c_str()))
        Serial.println("[DataStorage::addToHttpRequestsList] Error while opening HTTP request\r\n");
    else
        ahr->send();
    */

    return true;
}

bool removeFromHttpRequestsList(uint32_t pos)
{
    if (pos >= m_httpRequestsListSize)
    {
        Serial.printf("[DataStorage::removeFromHttpRequestsList] invalid index %u more then list size %u\r\n", pos, m_httpRequestsListSize);
        return false;
    }

    delete m_httpRequestsList[pos];
    --m_httpRequestsListSize;

    if (pos != m_httpRequestsListSize)
        m_httpRequestsList[pos] = m_httpRequestsList[m_httpRequestsListSize];

    return true;
}

void eraseHttpRequestsList()
{
    for (uint32_t i = 0; i < m_httpRequestsListSize; ++i)
        delete m_httpRequestsList[i];

    m_httpRequestsListSize = 0;
}

uint32_t httpRequestsListSize()
{
    return m_httpRequestsListSize;
}

void removeAllCompletedHttpRequests()
{
    uint32_t i = 0;
    while (i < httpRequestsListSize())
    {
        m_httpRequestsList[i]->userPoll();

        // if (m_httpRequestsList[i]->readyState() == asyncHTTPrequest::readyStateDone)
        if (m_httpRequestsList[i]->getState() == LWIP_HTTPRequest::Closed)
            removeFromHttpRequestsList(i);
        else
            ++i;
    }
}
// -----=====+++++oooooOOOOO End of HTTP requests list functions OOOOOooooo+++++=====-----

// -----=====+++++oooooOOOOO Public interface OOOOOooooo+++++=====-----
void setup(const char *ssid, const char *passwd, Blinker *gb, Blinker *rb)
{
    m_macAddress = WiFi.macAddress();
    m_greenLed = gb;
    m_redLed = rb;

    m_onConnectedHandler = WiFi.onStationModeConnected(onConnected);
    m_onDisconnectedHandler = WiFi.onStationModeDisconnected(onDisconnected);
    m_onAuthModeChangedHandler = WiFi.onStationModeAuthModeChanged(onAuthModeChanged);
    m_onGotIPHandler = WiFi.onStationModeGotIP(onGotIP);
    m_onDHCPTimeoutHandler = WiFi.onStationModeDHCPTimeout(onDHCPTimeout);

    Serial.printf("[WIFI] Attempting to connect to %s\r\n", ssid);
    WiFi.begin(ssid, passwd);

    m_greenLed->setMode(Blinker::Data);
    m_redLed->setMode(Blinker::Off);
}

void work()
{
    removeAllCompletedHttpRequests();
}

void incrementCounters(bool cold, bool hot)
{
    if (cold)
        m_coldWaterCounter += WATER_COLD_INCREMENT;

    if (hot)
        m_hotWaterCounter += WATER_HOT_INCREMENT;

    Serial.printf("[DataStorage::incrementCounters %lu] Attempting to send HTTP data", millis());

    if (cold)
        Serial.printf(" COLD = %u", m_coldWaterCounter);

    if (hot)
        Serial.printf(" HOT = %u", m_hotWaterCounter);

    Serial.println();

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("[DataStorage::incrementCounters] No Wi-Fi connections available, can't send data");
        return;
    }

    // String url = "http://salieff.phantomazz.me:5190/jjrpulser/text.sh?cmd=add_value&mac=" + m_macAddress;
    // String url = "http://46.39.250.254:5190/jjrpulser/text.sh?cmd=add_value&mac=" + m_macAddress;
    String url = "/jjrpulser/text.sh?cmd=add_value&mac=" + m_macAddress;

    if (cold)
    {
        url += "&cold=";
        url += String(m_coldWaterCounter);
    }

    if (hot)
    {
        url += "&hot=";
        url += String(m_hotWaterCounter);
    }

    if (addToHttpRequestsList("salieff.phantomazz.me", 5190, url))
        Serial.printf("[DataStorage::incrementCounters] requests list size %u\r\n", httpRequestsListSize());
}

uint32_t httpErrors()
{
    return m_httpErrorsCounter;
}
// -----=====+++++oooooOOOOO End of Public interface OOOOOooooo+++++=====-----

}; // namespace DataStorage
