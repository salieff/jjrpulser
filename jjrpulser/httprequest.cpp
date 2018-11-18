#include "httprequest.h"

extern "C" {
#include <lwip/dns.h>
}

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
}

void LWIP_HTTPRequest::userPoll()
{
    unsigned long delta = millis() - m_lastPollTimestamp;
    if (delta < 100) // 10 times per second
        return;

    m_lastPollTimestamp = millis();

    switch(m_state)
    {
        case ResolveFailed :
            break;

        case ConnectFailed :
            break;

        case SentFailed :
            break;

        case RecvFailed :
            break;

        case CloseFailed :
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
    m_lastError = dns_gethostbyname(m_host.c_str(), &addr, [](const char *, ip_addr_t *ipaddr, void *arg){ (static_cast<LWIP_HTTPRequest *>(arg))->onDnsFound(ipaddr); }, this);
    if (m_lastError == ERR_OK)
    {
        connect(&addr);
        return;
    }

    if (m_lastError == ERR_INPROGRESS)
        return;

    m_state = ResolveFailed;
}

void LWIP_HTTPRequest::onDnsFound(ip_addr_t *ipaddr)
{
    if ((ipaddr) && (ipaddr->addr))
    {
        connect(ipaddr);
        return;
    }

    m_state = ResolveFailed;
}

void LWIP_HTTPRequest::connect(ip_addr_t *ipaddr)
{
    m_state = Connecting;

    m_clientPcb = tcp_new();
    if (!m_clientPcb)
    {
        m_lastError = ERR_MEM;
        m_state = ConnectFailed;
        return;
    }

    tcp_arg(m_clientPcb, this);
    tcp_err(m_clientPcb, [](void *arg, err_t err){ return (static_cast<LWIP_HTTPRequest *>(arg))->onTcpError(err); });

    m_lastError = tcp_connect(m_clientPcb, ipaddr, m_port, [](void *arg, tcp_pcb *, err_t err) -> err_t { return (static_cast<LWIP_HTTPRequest *>(arg))->onTcpConnected(err); });
    if (m_lastError == ERR_OK)
        return;

    m_state = ConnectFailed;
}

err_t LWIP_HTTPRequest::onTcpConnected(err_t err) // An unused error code, always ERR_OK currently ;-)
{
    if (err != ERR_OK)
    {
        m_lastError = err;
        m_state = ConnectFailed;
        return err;
    }

    tcp_sent(m_clientPcb, [](void *arg, tcp_pcb *, u16_t len) -> err_t { return (static_cast<LWIP_HTTPRequest *>(arg))->onTcpDataSent(len); });
    tcp_recv(m_clientPcb, [](void *arg, tcp_pcb *, pbuf *p, err_t err) -> err_t { return (static_cast<LWIP_HTTPRequest *>(arg))->onTcpDataReceived(p, err); });

    send();
    return ERR_OK;
}

void LWIP_HTTPRequest::onTcpError(err_t err)
{
    if (err == ERR_OK)
        return;

    m_lastError = err;
    m_state = ConnectFailed;
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
        return;
    }

    if (len > m_stringToSend.length())
        len = m_stringToSend.length();

    m_lastError = tcp_write(m_clientPcb, m_stringToSend.c_str(), len, 0);
    if (m_lastError == ERR_OK)
        return;

    m_state = SentFailed;
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
        m_lastError = err;
        m_state = RecvFailed;
        return err;
    }

    if (p->tot_len == 0)
    {
        pbuf_free(p);
        return ERR_OK;
    }

    char *tmpBuf = (char *)malloc(p->tot_len + 1);
    if (tmpBuf == nullptr)
    {
        m_lastError = ERR_MEM;
        m_state = RecvFailed;

        pbuf_free(p);
        return ERR_OK;
    }

    u16_t cpLen = pbuf_copy_partial(p, tmpBuf, p->tot_len, 0);
    tmpBuf[cpLen] = 0;

    m_stringForRecv += const_cast<const char *>(tmpBuf);

    free(tmpBuf);
    tcp_recved(m_clientPcb, cpLen);

    pbuf_free(p);
    return ERR_OK;
}

void LWIP_HTTPRequest::close()
{
    if (!m_clientPcb)
        return;

    m_state = Closing;

    tcp_arg(m_clientPcb, nullptr);
    tcp_sent(m_clientPcb, nullptr);
    tcp_recv(m_clientPcb, nullptr);
    tcp_err(m_clientPcb, nullptr);
    tcp_poll(m_clientPcb, nullptr, 0);

    m_lastError = tcp_close(m_clientPcb);
    if (m_lastError != ERR_OK)
    {
        m_state = CloseFailed;
        return;
    }

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
