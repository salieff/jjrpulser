#include "httprequest.h"

LWIP_HTTPRequest::LWIP_HTTPRequest(const char *host, uint16_t port, const char *url, ResultCallback cb, void *cbArg)
    : m_host(host)
    , m_port(port)
    , m_url(url)
    , m_resultCallback(cb)
    , m_resultCallbackArg(cbArg)
    , m_clientPcb(nullptr)
    , m_requestCode(ERR_OK)
{
    resolve();
}

LWIP_HTTPRequest::~LWIP_HTTPRequest()
{
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
    ip_addr_t addr;
    err_t err = dns_gethostbyname(m_host.c_str(), &addr, [](const char *, ip_addr_t *ipaddr, void *arg){ (static_cast<LWIP_HTTPRequest *>(arg))->onDnsFound(ipaddr); }, this);
    if (err == ERR_OK)
    {
        connect(addr);
        return;
    }

    if (err == ERR_INPROGRESS)
        return;

    close();
    setRequestCode(err);
}

void LWIP_HTTPRequest::onDnsFound(ip_addr_t *ipaddr)
{
    if ((ipaddr) && (ipaddr->addr))
    {
        connect(ipaddr);
        return;
    }

    close();
    setRequestCode(ERR_ARG);
}

void LWIP_HTTPRequest::connect(ip_addr_t *ipaddr)
{
    m_clientPcb = tcp_new();
    if (!m_clientPcb)
    {
        setRequestCode(ERR_MEM);
        return;
    }

    tcp_arg(m_clientPcb, this);
    tcp_err(m_clientPcb, [](void *arg, err_t err){ return (static_cast<LWIP_HTTPRequest *>(arg))->onTcpError(err); });

    err_t err = tcp_connect(m_clientPcb, ipaddr, m_port, [](void *arg, tcp_pcb *, err_t err) -> err_t { return (static_cast<LWIP_HTTPRequest *>(arg))->onTcpConnected(err); });
    if (err != ERR_OK)
    {
        setRequestCode(err);
        close();
        return;
    }
}

err_t LWIP_HTTPRequest::send()
{
    if (m_stringToSend.length() == 0)
        return ERR_OK;

    u16_t len = tcp_sndbuf(m_clientPcb);
    if (len == 0)
        return ERR_MEM;

    if (len > m_stringToSend.length())
        len = m_stringToSend.length();

    return tcp_write(m_clientPcb, m_stringToSend.c_str(), len, 0);
}

void LWIP_HTTPRequest::onTcpError(err_t err)
{
        setRequestCode(err);
        close();
}

err_t LWIP_HTTPRequest::onTcpConnected(err_t err)
{
    tcp_sent(m_clientPcb, [](void *arg, tcp_pcb *, u16_t len) -> err_t { return (static_cast<LWIP_HTTPRequest *>(arg))->onTcpDataSent(len); });
    tcp_recv(m_clientPcb, [](void *arg, tcp_pcb *, pbuf *p, err_t err) -> err_t { return (static_cast<LWIP_HTTPRequest *>(arg))->onTcpDataReceived(p, err); });

    return send();
}

err_t LWIP_HTTPRequest::onTcpDataSent(u16_t len)
{
    m_stringToSend.remove(0, len);
    return send();
}

err_t LWIP_HTTPRequest::onTcpDataReceived(pbuf *p, err_t err)
{
}

err_t LWIP_HTTPRequest::close()
{
    if (!m_clientPcb)
        return ERR_OK;

    tcp_arg(m_clientPcb, nullptr);
    tcp_sent(m_clientPcb, nullptr);
    tcp_recv(m_clientPcb, nullptr);
    tcp_err(m_clientPcb, nullptr);
    tcp_poll(m_clientPcb, nullptr, 0);

    err_t err = tcp_close(m_clientPcb);
    if (err != ERR_OK)
    {
        tcp_abort(m_clientPcb);
        err = ERR_ABRT;
    }

    m_clientPcb = nullptr;
    return err;
}

void LWIP_HTTPRequest::setRequestCode(int c)
{
    if (m_requestCode == c)
        return;

    m_requestCode = c;
    if (m_resultCallback)
        m_resultCallback(m_resultCallbackArg, this, m_requestCode);
}
