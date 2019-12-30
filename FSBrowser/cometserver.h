#ifndef JJR_COMET_SERVER_H
#define JJR_COMET_SERVER_H

#include <ESP8266WebServer.h>

class CometServer {
public:
    CometServer(int port = 8080);

    void setup();
    void work();

    void onSubscribe();
    void postEvent(String e);

private:
    void finalizeAndSend(WiFiClient &client);

    ESP8266WebServer m_server;
    String m_payload;
    bool m_needHandleClient;
};

#endif // JJR_COMET_SERVER_H
