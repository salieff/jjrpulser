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

    typedef void (*ResultCallback)(void *, LWIP_HTTPRequest *, int, const String &);

    LWIP_HTTPRequest(const char *host, uint16_t port, const char *url, ResultCallback cb = nullptr, void *cbArg = nullptr);
    ~LWIP_HTTPRequest();

    void userPoll();
    void markForDelete();
    bool markedForDelete() const;

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
    void connect(const ip_addr_t *ipaddr);
    void send();
    void close();
    void abort();

    void onDnsFound(const ip_addr_t *ipaddr);
    void onTcpError(err_t err);
    err_t onTcpConnected(err_t err); // An unused error code, always ERR_OK currently ;-)
    err_t onTcpDataSent(u16_t len);
    err_t onTcpDataReceived(pbuf *p, err_t err);

    void processNewLine(const String &str);
    bool processStartLine(const String &str);
    bool processHeaderLine(const String &str);
    bool processBodyLine(const String &str);

    int getNextToken(const String &inStr, String &outStr, int startInd = 0, const String &sepStr = String(" \t\v\f\r\n"));
    bool tokenToInt(const String &token, int &i);

    bool receivedCorrectHttp() const;
    void fireCallback(bool byTimeout = false);

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

    bool m_markedForDelete;
};

#endif // JJR_PULSER_HTTP_REQUEST_H
