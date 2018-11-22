#include "httprequest.h"

extern "C" {
#include <lwip/tcp.h>
#include <lwip/dns.h>
}

#include <errno.h>

#ifdef DEBUG_ESP_HTTP_CLIENT
#ifdef DEBUG_ESP_PORT
#define DEBUG_LWIP_HTTPREQUEST(...) DEBUG_ESP_PORT.printf( __VA_ARGS__ )
#endif
#endif

#ifndef DEBUG_LWIP_HTTPREQUEST
#define DEBUG_LWIP_HTTPREQUEST(...)
#endif

LWIP_HTTPRequest::LWIP_HTTPRequest(const char *host, uint16_t port, const char *url, ResultCallback cb, void *cbArg)
    : m_host(host)
    , m_port(port)
    , m_url(url)
    , m_resultCallback(cb)
    , m_resultCallbackArg(cbArg)
    , m_clientPcb(nullptr)
    , m_lastError(ERR_OK)
    , m_state(Closed)
    , m_httpCode(-1)
    , m_contentLength(-1)
    , m_bodyStarted(false)
    , m_lastPollTimestamp(millis())
    , m_constructTimestamp(millis())
    , m_markedForDelete(false)
{
    resolve();
}

LWIP_HTTPRequest::~LWIP_HTTPRequest()
{
    close();

    if (m_state != Closed)
        abort();

    DEBUG_LWIP_HTTPREQUEST("[LWIP_HTTPRequest::~LWIP_HTTPRequest] I was destructed\r\n");
}

void LWIP_HTTPRequest::userPoll()
{
    if (markedForDelete())
        return;

    unsigned long delta = millis() - m_lastPollTimestamp;
    if (delta < 100) // 10 times per second
        return;

    m_lastPollTimestamp = millis();

    delta = millis() - m_constructTimestamp;
    if (delta >= LWIP_HTTP_REQUEST_TIMEOUT)
    {
        fireCallback(!receivedCorrectHttp());
        return;
    }

    switch(m_state)
    {
    case ResolveFailed :
        resolve();
        break;

    case ConnectFailed :
        close();
        break;

    case SentFailed :
        if (ERR_IS_FATAL(m_lastError))
            close();
        else
            send();

        break;

    case RecvFailed :
        close();
        break;

    case CloseFailed :
        if (ERR_IS_FATAL(m_lastError))
            close();
        else
            abort();

        break;

    case Closed :
        if (!receivedCorrectHttp())
            resolve();
        else
            fireCallback();

        break;

    case Resolving :
    case Connecting :
    case Sending :
    case Receiving :
    case Closing :
        // Do nothing
        break;
    }
}

void LWIP_HTTPRequest::markForDelete()
{
    m_markedForDelete = true;
}

bool LWIP_HTTPRequest::markedForDelete() const
{
    return m_markedForDelete;
}

void LWIP_HTTPRequest::constructRequest()
{
    m_stringToSend  = "GET ";
    m_stringToSend += m_url;
    m_stringToSend += " HTTP/1.1\r\n";
    m_stringToSend += "Host: ";
    m_stringToSend += m_host;
    m_stringToSend += "\r\n";
    m_stringToSend += "Connection: close\r\n\r\n";

    m_stringForRecv = "";

    m_httpCode = -1;
    m_contentLength = -1;

    m_bodyStarted = false;
    m_httpBody = "";
}

void LWIP_HTTPRequest::resolve()
{
    m_state = Resolving;

    ip_addr_t addr;
    m_lastError = dns_gethostbyname(m_host.c_str(), &addr, [](const char *, const ip_addr_t *ipaddr, void *arg) {
        (static_cast<LWIP_HTTPRequest *>(arg))->onDnsFound(ipaddr);
    }, this);
    if (m_lastError == ERR_OK)
    {
        connect(&addr);
        return;
    }

    if (m_lastError == ERR_INPROGRESS)
        return;

    m_state = ResolveFailed;
    DEBUG_LWIP_HTTPREQUEST("[LWIP_HTTPRequest::resolve] ResolveFailed 1 %ld\r\n", m_lastError);
}

void LWIP_HTTPRequest::onDnsFound(const ip_addr_t *ipaddr)
{
    if ((ipaddr) && (ipaddr->addr))
    {
        connect(ipaddr);
        return;
    }

    m_state = ResolveFailed;
    m_lastError = ERR_VAL;
    DEBUG_LWIP_HTTPREQUEST("[LWIP_HTTPRequest::onDnsFound] ResolveFailed 2 %ld\r\n", m_lastError);
}

void LWIP_HTTPRequest::connect(const ip_addr_t *ipaddr)
{
    m_state = Connecting;

    m_clientPcb = tcp_new();
    if (!m_clientPcb)
    {
        m_lastError = ERR_MEM;
        m_state = ConnectFailed;
        DEBUG_LWIP_HTTPREQUEST("[LWIP_HTTPRequest::connect] ConnectFailed 1 %ld\r\n", m_lastError);
        return;
    }

    tcp_arg(m_clientPcb, this);
    tcp_err(m_clientPcb, [](void *arg, err_t err) {
        return (static_cast<LWIP_HTTPRequest *>(arg))->onTcpError(err);
    });

    m_lastError = tcp_connect(m_clientPcb, ipaddr, m_port, [](void *arg, tcp_pcb *, err_t err) -> err_t {
        return (static_cast<LWIP_HTTPRequest *>(arg))->onTcpConnected(err);
    });
    if (m_lastError == ERR_OK)
        return;

    m_state = ConnectFailed;
    DEBUG_LWIP_HTTPREQUEST("[LWIP_HTTPRequest::connect] ConnectFailed 2 %ld\r\n", m_lastError);
}

err_t LWIP_HTTPRequest::onTcpConnected(err_t err)
{
    if (err != ERR_OK) // An unused error code, always ERR_OK currently ;-)
    {
        m_lastError = err;
        m_state = ConnectFailed;
        DEBUG_LWIP_HTTPREQUEST("[LWIP_HTTPRequest::onTcpConnected] ConnectFailed 3 %ld\r\n", m_lastError);
        return err;
    }

    tcp_sent(m_clientPcb, [](void *arg, tcp_pcb *, u16_t len) -> err_t {
        return (static_cast<LWIP_HTTPRequest *>(arg))->onTcpDataSent(len);
    });
    tcp_recv(m_clientPcb, [](void *arg, tcp_pcb *, pbuf *p, err_t err) -> err_t {
        return (static_cast<LWIP_HTTPRequest *>(arg))->onTcpDataReceived(p, err);
    });

    constructRequest();

    send();
    return ERR_OK;
}

void LWIP_HTTPRequest::onTcpError(err_t err)
{
    if (err == ERR_OK)
        return;

    if (m_clientPcb)
    {
        tcp_arg(m_clientPcb, nullptr);
        tcp_sent(m_clientPcb, nullptr);
        tcp_recv(m_clientPcb, nullptr);
        tcp_err(m_clientPcb, nullptr);

        m_clientPcb = nullptr;
    }

    m_lastError = err; // LWIP сам закрывает и освобождает m_clientPcb в этом случае
    m_state = Closed;  // если делать тут tcp_close(), то будет переезд по памяти и падение

    DEBUG_LWIP_HTTPREQUEST("[LWIP_HTTPRequest::onTcpError] ConnectFailed 4 %ld\r\n", m_lastError);
}

void LWIP_HTTPRequest::send()
{
    if (m_stringToSend.length() == 0)
    {
        m_state = Receiving;
        return;
    }

    m_state = Sending;

    u16_t len = tcp_sndbuf(m_clientPcb);
    if (len == 0)
    {
        m_lastError = ERR_MEM;
        m_state = SentFailed;
        DEBUG_LWIP_HTTPREQUEST("[LWIP_HTTPRequest::send] SentFailed 1 %ld\r\n", m_lastError);
        return;
    }

    if (len > m_stringToSend.length())
        len = m_stringToSend.length();

    m_lastError = tcp_write(m_clientPcb, m_stringToSend.c_str(), len, 0);
    if (m_lastError == ERR_OK)
        return;

    m_state = SentFailed;
    DEBUG_LWIP_HTTPREQUEST("[LWIP_HTTPRequest::send] SentFailed 2 %ld\r\n", m_lastError);
}

err_t LWIP_HTTPRequest::onTcpDataSent(u16_t len)
{
    m_stringToSend.remove(0, len);
    send();
    return ERR_OK;
}

err_t LWIP_HTTPRequest::onTcpDataReceived(pbuf *p, err_t err)
{
    DEBUG_LWIP_HTTPREQUEST("[LWIP_HTTPRequest::onTcpDataReceived %lu] Enter\r\n", millis());

    if (p == nullptr || err != ERR_OK)
    {
        // p == nullptr, это, по сути, не ошибка приема, а закрытие соединения другой стороной,
        // но нам неважно, мы оцениваем успешность приема на уровне HTTP, а не TCP
        m_lastError = err;
        m_state = RecvFailed;

        DEBUG_LWIP_HTTPREQUEST("[LWIP_HTTPRequest::onTcpDataReceived %lu] %s %ld\r\n", millis(), p ? "RecvFailed 1" : "Closed by server", m_lastError);
        return err;
    }

    if (p->tot_len == 0)
    {
        pbuf_free(p);
        DEBUG_LWIP_HTTPREQUEST("[LWIP_HTTPRequest::onTcpDataReceived] p->tot_len == 0\r\n");
        return ERR_OK;
    }

    char *tmpBuf = (char *)malloc(p->tot_len + 1);
    if (tmpBuf == nullptr)
    {
        m_lastError = ERR_MEM;
        m_state = RecvFailed;

        pbuf_free(p);

        DEBUG_LWIP_HTTPREQUEST("[LWIP_HTTPRequest::onTcpDataReceived] RecvFailed 2 %ld\r\n", m_lastError);
        return ERR_OK;
    }

    u16_t cpLen = pbuf_copy_partial(p, tmpBuf, p->tot_len, 0);
    tmpBuf[cpLen] = 0;

    m_stringForRecv += const_cast<const char *>(tmpBuf);
    DEBUG_LWIP_HTTPREQUEST("[LWIP_HTTPRequest::onTcpDataReceived %lu] m_stringForRecv = %s\r\n", millis(), m_stringForRecv.c_str());

    free(tmpBuf);
    tcp_recved(m_clientPcb, cpLen);

    pbuf_free(p);

    DEBUG_LWIP_HTTPREQUEST("[LWIP_HTTPRequest::onTcpDataReceived %lu] Start processing\r\n", millis());
    int ind = -1;
    while ((ind = m_stringForRecv.indexOf('\n')) >= 0)
    {
        processNewLine(m_stringForRecv.substring(0, ind + 1));
        m_stringForRecv.remove(0, ind + 1);
    }

    DEBUG_LWIP_HTTPREQUEST("[LWIP_HTTPRequest::onTcpDataReceived %lu] Leave\r\n", millis());
    return ERR_OK;
}

void LWIP_HTTPRequest::close()
{
    if (m_stringForRecv.length() > 0)
    {
        processNewLine(m_stringForRecv);
        m_stringForRecv = "";
    }

    if (!m_clientPcb)
    {
        m_state = Closed;
        return;
    }

    m_state = Closing;

    tcp_arg(m_clientPcb, nullptr);
    tcp_sent(m_clientPcb, nullptr);
    tcp_recv(m_clientPcb, nullptr);
    tcp_err(m_clientPcb, nullptr);

    m_lastError = tcp_close(m_clientPcb);
    if (m_lastError != ERR_OK)
    {
        m_state = CloseFailed;
        DEBUG_LWIP_HTTPREQUEST("[LWIP_HTTPRequest::close] CloseFailed 1 %ld\r\n", m_lastError);
        return;
    }

    m_clientPcb = nullptr;
    m_state = Closed;
}

void LWIP_HTTPRequest::abort() // If abort() was called from LWIP callback, it's needed to return ERR_ABRT
{
    if (m_stringForRecv.length() > 0)
    {
        processNewLine(m_stringForRecv);
        m_stringForRecv = "";
    }

    if (!m_clientPcb)
    {
        m_state = Closed;
        return;
    }

    tcp_arg(m_clientPcb, nullptr);
    tcp_sent(m_clientPcb, nullptr);
    tcp_recv(m_clientPcb, nullptr);
    tcp_err(m_clientPcb, nullptr);

    tcp_abort(m_clientPcb);

    m_clientPcb = nullptr;
    m_state = Closed;
}

void LWIP_HTTPRequest::processNewLine(const String &str)
{
    bool okFlag = true;

    if (m_httpCode < 0)
        okFlag = okFlag && processStartLine(str);
    else if (!m_bodyStarted)
        okFlag = okFlag && processHeaderLine(str);
    else
        okFlag = okFlag && processBodyLine(str);

    if (okFlag)
        return;

    // Если была хоть одна ошибка парсинга HTTP протокола, попадаем сюда
    // и инициируем ошибку приема, что приводит к закрытию соединения
    // и повторной попытке запроса, если HTTP ответ некорректен

    if (m_clientPcb)
        tcp_recv(m_clientPcb, nullptr);

    m_lastError = ERR_VAL;
    m_state = RecvFailed;
}

bool LWIP_HTTPRequest::processStartLine(const String &str)
{
    // HTTP/1.1 200 OK
    String token;

    int tokenEnd = getNextToken(str, token);
    if (tokenEnd < 0 || !token.startsWith("HTTP/"))
        return false;

    tokenEnd = getNextToken(str, token, tokenEnd);
    if (tokenEnd < 0 || !tokenToInt(token, m_httpCode))
        return false;

    return true;
}

bool LWIP_HTTPRequest::processHeaderLine(const String &str)
{
    if (str == "\r\n" || str == "\n")
    {
        m_bodyStarted = true;
        return true;
    }

    // Content-Length: 49
    String token;

    int tokenEnd = getNextToken(str, token, 0, ":");
    if (tokenEnd < 0 || (unsigned)tokenEnd == str.length())
        return false;

    token.trim();
    if (token.length() == 0)
        return false;

    if (!token.equalsIgnoreCase("Content-Length"))
        return true;

    tokenEnd = getNextToken(str, token, tokenEnd, ":");
    if (tokenEnd < 0)
        return false;

    token.trim();
    if (!tokenToInt(token, m_contentLength))
        return false;

    return true;
}

bool LWIP_HTTPRequest::processBodyLine(const String &str)
{
    m_httpBody += str;

    if (m_contentLength >= 0 && m_httpBody.length() > (unsigned)m_contentLength)
    {
        m_httpBody.remove(m_contentLength);
        return false;
    }

    return true;
}

bool LWIP_HTTPRequest::receivedCorrectHttp() const
{
    if (m_httpCode < 0)
        return false;

    if (!m_bodyStarted)
        return false;

    if (m_contentLength >= 0 && m_httpBody.length() != (unsigned)m_contentLength)
        return false;

    return true;
}

void LWIP_HTTPRequest::fireCallback(bool byTimeout)
{
    if (!m_resultCallback)
        return;

    if (byTimeout)
        m_resultCallback(m_resultCallbackArg, this, -1, "");
    else
        m_resultCallback(m_resultCallbackArg, this, m_httpCode, m_httpBody);

    m_resultCallback = nullptr;
    m_resultCallbackArg = nullptr;
}

int getNextToken(const String &inStr, String &outStr, int startInd, const String &sepStr)
{
    int tokenStart = -1;
    for (int i = startInd; (unsigned)i < inStr.length(); ++i)
        if (sepStr.indexOf(inStr[i]) < 0)
        {
            tokenStart = i;
            break;
        }

    if (tokenStart < 0)
    {
        outStr = "";
        return -1;
    }

    int tokenEnd = inStr.length();
    for (int i = tokenStart + 1; (unsigned)i < inStr.length(); ++i)
        if (sepStr.indexOf(inStr[i]) >= 0)
        {
            tokenEnd = i;
            break;
        }

    outStr = inStr.substring(tokenStart, tokenEnd);
    return tokenEnd;
}

bool tokenToInt(const String &token, int &i)
{
    if (token.length() == 0)
        return false;

    char *endptr = nullptr;
    errno = 0;
    i = strtol(token.c_str(), &endptr, 10);
    if (errno != 0 || *endptr != '\0')
    {
        i = -1;
        return false;
    }

    return true;
}
