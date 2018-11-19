#include "httprequest.h"

extern "C" {
#include <lwip/tcp.h>
#include <lwip/dns.h>
}

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
    , m_requestCode(ERR_OK)
    , m_lastPollTimestamp(millis())
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
    unsigned long delta = millis() - m_lastPollTimestamp;
    if (delta < 100) // 10 times per second
        return;

    m_lastPollTimestamp = millis();

    // TODO: Break state-machine by timeout or successfull result

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
    case Resolving :
    case Connecting :
    case Sending :
    case Receiving :
    case Closing :
        // Do nothing
        break;
    }
}

LWIP_HTTPRequest::State LWIP_HTTPRequest::getState() const
{
    return m_state;
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
}

void LWIP_HTTPRequest::resolve()
{
    m_state = Resolving;

    ip_addr_t addr;
    m_lastError = dns_gethostbyname(m_host.c_str(), &addr, [](const char *, ip_addr_t *ipaddr, void *arg) {
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
    DEBUG_LWIP_HTTPREQUEST("[LWIP_HTTPRequest::resolve] ResolveFailed 1 %d\r\n", m_lastError);
}

void LWIP_HTTPRequest::onDnsFound(ip_addr_t *ipaddr)
{
    if ((ipaddr) && (ipaddr->addr))
    {
        connect(ipaddr);
        return;
    }

    m_state = ResolveFailed;
    m_lastError = ERR_VAL;
    DEBUG_LWIP_HTTPREQUEST("[LWIP_HTTPRequest::onDnsFound] ResolveFailed 2 %d\r\n", m_lastError);
}

void LWIP_HTTPRequest::connect(ip_addr_t *ipaddr)
{
    m_state = Connecting;

    m_clientPcb = tcp_new();
    if (!m_clientPcb)
    {
        m_lastError = ERR_MEM;
        m_state = ConnectFailed;
        DEBUG_LWIP_HTTPREQUEST("[LWIP_HTTPRequest::connect] ConnectFailed 1 %d\r\n", m_lastError);
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
    DEBUG_LWIP_HTTPREQUEST("[LWIP_HTTPRequest::connect] ConnectFailed 2 %d\r\n", m_lastError);
}

err_t LWIP_HTTPRequest::onTcpConnected(err_t err)
{
    if (err != ERR_OK) // An unused error code, always ERR_OK currently ;-)
    {
        m_lastError = err;
        m_state = ConnectFailed;
        DEBUG_LWIP_HTTPREQUEST("[LWIP_HTTPRequest::onTcpConnected] ConnectFailed 3 %d\r\n", m_lastError);
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

    DEBUG_LWIP_HTTPREQUEST("[LWIP_HTTPRequest::onTcpError] ConnectFailed 4 %d\r\n", m_lastError);
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
        DEBUG_LWIP_HTTPREQUEST("[LWIP_HTTPRequest::send] SentFailed 1 %d\r\n", m_lastError);
        return;
    }

    if (len > m_stringToSend.length())
        len = m_stringToSend.length();

    m_lastError = tcp_write(m_clientPcb, m_stringToSend.c_str(), len, 0);
    if (m_lastError == ERR_OK)
        return;

    m_state = SentFailed;
    DEBUG_LWIP_HTTPREQUEST("[LWIP_HTTPRequest::send] SentFailed 2 %d\r\n", m_lastError);
}

err_t LWIP_HTTPRequest::onTcpDataSent(u16_t len)
{
    m_stringToSend.remove(0, len);
    send();
    return ERR_OK;
}

err_t LWIP_HTTPRequest::onTcpDataReceived(pbuf *p, err_t err)
{
    if (p == nullptr || err != ERR_OK)
    {
        // p == nullptr, это, по сути, не ошибка приема, а закрытие соединения другой стороной,
        // но нам неважно, мы оцениваем успешность приема на уровне HTTP, а не TCP
        m_lastError = err;
        m_state = RecvFailed;
        DEBUG_LWIP_HTTPREQUEST("[LWIP_HTTPRequest::onTcpDataReceived] RecvFailed 1 p = %p %d\r\n", p, m_lastError);
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

        DEBUG_LWIP_HTTPREQUEST("[LWIP_HTTPRequest::onTcpDataReceived] RecvFailed 2 %d\r\n", m_lastError);
        return ERR_OK;
    }

    u16_t cpLen = pbuf_copy_partial(p, tmpBuf, p->tot_len, 0);
    tmpBuf[cpLen] = 0;

    m_stringForRecv += const_cast<const char *>(tmpBuf);
    DEBUG_LWIP_HTTPREQUEST("[LWIP_HTTPRequest::onTcpDataReceived] m_stringForRecv = %s\r\n", m_stringForRecv.c_str());

    free(tmpBuf);
    tcp_recved(m_clientPcb, cpLen);

    pbuf_free(p);
    return ERR_OK;
}

void LWIP_HTTPRequest::close()
{
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
        DEBUG_LWIP_HTTPREQUEST("[LWIP_HTTPRequest::close] CloseFailed 1 %d\r\n", m_lastError);
        return;
    }

    m_clientPcb = nullptr;
    m_state = Closed;
}

void LWIP_HTTPRequest::abort() // If abort() was called from LWIP callback, it's needed to return ERR_ABRT
{
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

void LWIP_HTTPRequest::setRequestCode(int c)
{
    if (m_requestCode == c)
        return;

    m_requestCode = c;
    if (m_resultCallback)
        m_resultCallback(m_resultCallbackArg, this, m_requestCode);
}
