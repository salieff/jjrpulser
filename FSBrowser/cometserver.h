#ifndef JJR_COMET_SERVER_H
#define JJR_COMET_SERVER_H

#include <ESP8266WebServer.h>

class CometServer {
public:
    CometServer(int port = 8080);

    void setup();
    void work();

private:
    ESP8266WebServer m_server;
    String m_payload;
};

#endif // JJR_COMET_SERVER_H
