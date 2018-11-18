#ifndef JJR_PULSER_HTTP_REQUEST_H
#define JJR_PULSER_HTTP_REQUEST_H

#include <Arduino.h>

extern "C" {
#include <lwip/tcp.h>
}

class LWIP_HTTPRequest {
public:
    typedef void (*ResultCallback)(void *, LWIP_HTTPRequest *, int);

    LWIP_HTTPRequest(const char *host, uint16_t port, const char *url, ResultCallback cb = nullptr, void *cbArg = nullptr);
    ~LWIP_HTTPRequest();

    void userPoll();

private:
    enum State {
        Closed,
        Resolving,
        ResolveFailed,
        Connecting,
        ConnectFailed,
        Sending,
        SentFailed,
        Receiving,
        RecvFailed,
        Closing,
        CloseFailed
    };

    void constructRequest();
    void resolve();
    void connect(ip_addr_t *ipaddr);
    void send();
    void close();

    void onDnsFound(ip_addr_t *ipaddr);
    void onTcpError(err_t err);
    err_t onTcpConnected(err_t err); // An unused error code, always ERR_OK currently ;-)
    err_t onTcpDataSent(u16_t len);
    err_t onTcpDataReceived(pbuf *p, err_t err);

    void setRequestCode(int c);

    String m_host;
    uint16_t m_port;
    String m_url;
    String m_stringToSend;
    String m_stringForRecv;

    ResultCallback m_resultCallback;
    void * m_resultCallbackArg;

    tcp_pcb *m_clientPcb;
    err_t m_lastError;
    State m_state;
    int m_requestCode;

    unsigned long m_lastPollTimestamp;
};

#endif // JJR_PULSER_HTTP_REQUEST_H
