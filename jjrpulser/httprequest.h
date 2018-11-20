#ifndef JJR_PULSER_HTTP_REQUEST_H
#define JJR_PULSER_HTTP_REQUEST_H

#include <Arduino.h>

extern "C" {
#include <lwip/err.h>
#include <lwip/ip_addr.h>
}

#define LWIP_HTTP_REQUEST_TIMEOUT 60000

struct pbuf;
struct tcp_pcb;

class LWIP_HTTPRequest {
public:
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

    typedef void (*ResultCallback)(void *, LWIP_HTTPRequest *, int, String &);

    LWIP_HTTPRequest(const char *host, uint16_t port, const char *url, ResultCallback cb = nullptr, void *cbArg = nullptr);
    ~LWIP_HTTPRequest();

    void userPoll();
    // State getState() const;

private:

    void constructRequest();
    void resolve();
    void connect(ip_addr_t *ipaddr);
    void send();
    void close();
    void abort();

    void onDnsFound(ip_addr_t *ipaddr);
    void onTcpError(err_t err);
    err_t onTcpConnected(err_t err); // An unused error code, always ERR_OK currently ;-)
    err_t onTcpDataSent(u16_t len);
    err_t onTcpDataReceived(pbuf *p, err_t err);

    void processNewLine(String &str);
    bool processStartLine(String &str);
    bool processHeaderLine(String &str);
    bool processBodyLine(String &str);

    int getNextToken(String &inStr, String &outStr, int startInd = 0, String &sepStr = String(" \t\v\f\r\n"));
    bool tokenToInt(String &token, int &i);

    bool receivedCorrectHttp() const;

    String m_host;
    uint16_t m_port;
    String m_url;

    String m_stringToSend;
    String m_stringForRecv;

    ResultCallback m_resultCallback;
    void *m_resultCallbackArg;

    tcp_pcb *m_clientPcb;
    err_t m_lastError;
    State m_state;

    int m_httpCode;
    int m_contentLength;
    bool m_bodyStarted;
    String m_httpBody;

    unsigned long m_lastPollTimestamp;
    unsigned long m_constructTimestamp;
};

#endif // JJR_PULSER_HTTP_REQUEST_H
