#include "storage.h"
#include "httprequest.h"
#include "passwords.h"

#include <EEPROM.h>

namespace DataStorage {

Blinker *m_greenLed = NULL;
Blinker *m_redLed = NULL;

struct {
    uint32_t m_coldWaterCounter;
    uint32_t m_hotWaterCounter;
    uint32_t m_crc32;
} m_counters = {0, 0, 0};

struct {
    unsigned long m_lastMillis;

    unsigned long m_upTimeDays;
    unsigned long m_upTimeHours;
    unsigned long m_upTimeMinutes;
    unsigned long m_upTimeSeconds;
    unsigned long m_upTimeMillis;

    uint32_t m_lastFreeHeap;

    uint32_t m_httpReqSent;
    uint32_t m_httpReqCommited;
    uint32_t m_httpReqFailed;
} m_statistics = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

struct {
    WiFiEventHandler m_onConnectedHandler;
    WiFiEventHandler m_onDisconnectedHandler;
    WiFiEventHandler m_onAuthModeChangedHandler;
    WiFiEventHandler m_onGotIPHandler;
    WiFiEventHandler m_onDHCPTimeoutHandler;
} m_wifiHandlers = {NULL, NULL, NULL, NULL, NULL};

uint32_t m_httpRequestsListSize = 0;
LWIP_HTTPRequest *m_httpRequestsList[HTTP_CONN_LIST_MAX];

String m_macAddress;

// -----=====+++++oooooOOOOO EEPROM Utilites OOOOOooooo+++++=====-----
static uint32_t m_CRCTable[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c, 0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};

uint32_t updateCRC32(uint32_t crc, uint8_t data)
{
    uint8_t idx = crc ^ data;
    crc = m_CRCTable[idx & 0x0f] ^ (crc >> 4);

    idx = crc ^ (data >> 4);
    crc = m_CRCTable[idx & 0x0f] ^ (crc >> 4);

    return crc;
}

uint32_t calculateCRC32(const uint8_t *buf, int len)
{
    uint32_t crc = ~0L;

    for (int i = 0; i < len; ++i)
        crc = updateCRC32(crc, *buf++);

    crc = ~crc;
    return crc;
}

void loadEEPROMSettings()
{
    EEPROM.get(0, m_counters);
    uint32_t crc = calculateCRC32(reinterpret_cast<uint8_t *>(&m_counters), sizeof(m_counters.m_coldWaterCounter) + sizeof(m_counters.m_hotWaterCounter));
    if (m_counters.m_crc32 == crc)
    {
        Serial.printf("[EEPROM] Loaded COLD  = %u\r\n", m_counters.m_coldWaterCounter);
        Serial.printf("                HOT   = %u\r\n", m_counters.m_hotWaterCounter);
        Serial.printf("                CRC32 = 0x%08xu\r\n", m_counters.m_crc32);
        return;
    }

    m_counters.m_coldWaterCounter = 0;
    m_counters.m_hotWaterCounter = 0;
    m_counters.m_crc32 = 0;

    Serial.printf("[EEPROM] Bad CRC32 set all data to null\r\n");
}

void saveEEPROMSettings()
{
    m_counters.m_crc32 = calculateCRC32(reinterpret_cast<uint8_t *>(&m_counters), sizeof(m_counters.m_coldWaterCounter) + sizeof(m_counters.m_hotWaterCounter));
    EEPROM.put(0, m_counters);
    EEPROM.commit();
}

// -----=====+++++oooooOOOOO End of EEPROM Utilites OOOOOooooo+++++=====-----

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
void checkSetupAttr(const String &line, const char *attr, int &retInt)
{
    String token;

    int tokenEnd = getNextToken(line, token, 0, "=");
    if (tokenEnd < 0 || (unsigned)tokenEnd == line.length())
        return;

    token.trim();
    if (token.length() == 0)
        return;

    if (!token.equalsIgnoreCase(attr))
        return;

    tokenEnd = getNextToken(line, token, tokenEnd, "=");
    if (tokenEnd < 0)
        return;

    token.trim();
    tokenToInt(token, retInt);
}

bool checkSetupColdHot(const String &body)
{
    int newCold = -1;
    int newHot = -1;

    int startInd = 0;
    int endInd = 0;

    do
    {
        endInd = body.indexOf('\n', startInd);
        String line = (endInd >= 0) ? body.substring(startInd, endInd) : body.substring(startInd);
        startInd = endInd + 1;

        checkSetupAttr(line, "setup_new_cold", newCold);
        checkSetupAttr(line, "setup_new_hot", newHot);
    } while (endInd >= 0);

    if (newCold >= 0 || newHot >= 0)
    {
        Serial.print("    >>>> Incoming setup from server:");

        if (newCold >= 0)
        {
            Serial.printf(" COLD = %d", newCold);
            m_counters.m_coldWaterCounter = newCold;
        }

        if (newHot >= 0)
        {
            Serial.printf(" HOT = %d", newHot);
            m_counters.m_hotWaterCounter = newHot;
        }

        Serial.println();
        saveEEPROMSettings();
        return true;
    }

    return false;
}

void httpStateChanged(void *, LWIP_HTTPRequest *r, int code, const String &body)
{
    r->markForDelete();

    if (code >= 0)
        ++m_statistics.m_httpReqCommited;

    Serial.printf("[DataStorage::onHTTPStateChanged %lu] HTTP code : %d\r\n", millis(), code);
    Serial.printf("%s\r\n", body.c_str());
    if (code != 200)
    {
        ++m_statistics.m_httpReqFailed;
        m_redLed->setMode(Blinker::Error);
        return;
    }

    m_redLed->setMode(Blinker::Off);

    if (checkSetupColdHot(body))
        m_greenLed->setMode(Blinker::Setup);
    // else
    //    m_greenLed->setMode(Blinker::Work);
}

bool addToHttpRequestsList(const char *host, uint16_t port, String &url)
{
    if (m_httpRequestsListSize >= HTTP_CONN_LIST_MAX)
    {
        Serial.printf("[DataStorage::addToHttpRequestsList] List max size %d reached\r\n", HTTP_CONN_LIST_MAX);
        return false;
    }

    LWIP_HTTPRequest *ahr = new LWIP_HTTPRequest(host, port, url.c_str(), httpStateChanged);

    m_httpRequestsList[m_httpRequestsListSize] = ahr;
    ++m_httpRequestsListSize;

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
        memmove(m_httpRequestsList + pos, m_httpRequestsList + pos + 1, (m_httpRequestsListSize - pos) * sizeof(m_httpRequestsList[0]));

    return true;
}

void eraseHttpRequestsList()
{
    for (uint32_t i = 0; i < m_httpRequestsListSize; ++i)
        delete m_httpRequestsList[i];

    m_httpRequestsListSize = 0;
}

void removeAllCompletedHttpRequests()
{
    uint32_t i = 0;
    uint32_t removed = 0;

    while (i < m_httpRequestsListSize)
    {
        m_httpRequestsList[i]->userPoll();

        if (m_httpRequestsList[i]->markedForDelete())
        {
            removeFromHttpRequestsList(i);
            ++removed;
        }
        else
        {
            ++i;
        }
    }

    if (removed)
        Serial.printf("[removeAllCompletedHttpRequests] removed %u connections%s\r\n", removed, removed > 6 ? " LOOP WAS STUCK?" : "");
}
// -----=====+++++oooooOOOOO End of HTTP requests list functions OOOOOooooo+++++=====-----

// -----=====+++++oooooOOOOO Public interface OOOOOooooo+++++=====-----
void setup(const char *ssid, const char *passwd, Blinker *gb, Blinker *rb)
{
    EEPROM.begin(sizeof(m_counters));
    loadEEPROMSettings();

    m_macAddress = WiFi.macAddress();
    m_greenLed = gb;
    m_redLed = rb;

    m_wifiHandlers.m_onConnectedHandler = WiFi.onStationModeConnected(onConnected);
    m_wifiHandlers.m_onDisconnectedHandler = WiFi.onStationModeDisconnected(onDisconnected);
    m_wifiHandlers.m_onAuthModeChangedHandler = WiFi.onStationModeAuthModeChanged(onAuthModeChanged);
    m_wifiHandlers.m_onGotIPHandler = WiFi.onStationModeGotIP(onGotIP);
    m_wifiHandlers.m_onDHCPTimeoutHandler = WiFi.onStationModeDHCPTimeout(onDHCPTimeout);

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
        m_counters.m_coldWaterCounter += WATER_COLD_INCREMENT;

    if (hot)
        m_counters.m_hotWaterCounter += WATER_HOT_INCREMENT;

    saveEEPROMSettings();

    Serial.printf("[DataStorage::incrementCounters %lu] Attempting to send HTTP data", millis());

    if (cold)
        Serial.printf(" COLD = %u", m_counters.m_coldWaterCounter);

    if (hot)
        Serial.printf(" HOT = %u", m_counters.m_hotWaterCounter);

    Serial.println();

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("[DataStorage::incrementCounters] No Wi-Fi connections available, can't send data");
        m_redLed->setMode(Blinker::Error);
        return;
    }

    String url(JJR_PULSER_SERVER_URL);
    url += "?cmd=add_value&mac=";
    url += m_macAddress;

    if (cold)
    {
        url += "&cold=";
        url += String(m_counters.m_coldWaterCounter);
    }

    if (hot)
    {
        url += "&hot=";
        url += String(m_counters.m_hotWaterCounter);
    }

    if (addToHttpRequestsList(JJR_PULSER_SERVER, JJR_PULSER_SERVER_PORT, url))
    {
        ++m_statistics.m_httpReqSent;
        m_greenLed->setMode(Blinker::Data);
    }
    else
    {
        m_redLed->setMode(Blinker::Error);
    }
}

void printStatistics()
{
    Serial.printf("HTTP Requests sent: %u\r\n", m_statistics.m_httpReqSent);
    Serial.printf("HTTP Requests commited: %u\r\n", m_statistics.m_httpReqCommited);
    Serial.printf("HTTP Requests failed: %u\r\n", m_statistics.m_httpReqFailed);
    Serial.printf("HTTP Queue size %u\r\n", m_httpRequestsListSize);
}
// -----=====+++++oooooOOOOO End of Public interface OOOOOooooo+++++=====-----

}; // namespace DataStorage
