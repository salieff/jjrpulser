#include "cometserver.h"

CometServer::CometServer(int port)
    : m_server(port)
    , m_needHandleClient(true)
{
}

void CometServer::setup()
{
    m_server.on("/subscribe", HTTP_GET, [this]() { this->onSubscribe(); });
    m_server.begin();
}

void CometServer::work()
{
    if (!m_needHandleClient)
    {
        WiFiClient client = m_server.client();
        if (!client || !client.connected())
            m_needHandleClient = true;
    }

    if (m_needHandleClient)
        m_server.handleClient();
}

void CometServer::onSubscribe()
{
    WiFiClient client = m_server.client();
    if (!client)
        return;

    if (!client.connected())
        return;

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/json");
    client.println("Cache-Control: no-cache, must-revalidate");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Connection: close");
    client.flush();

    m_needHandleClient = false;

    if (m_payload.length() != 0)
        finalizeAndSend(client);
}

void CometServer::postEvent(String e)
{
    if (m_payload.length() == 0)
        m_payload = "{\"events\": [";
    else
        m_payload += ",";

    m_payload += e;

    WiFiClient client = m_server.client();
    if (!client)
        return;

    if (!client.connected())
        return;

    finalizeAndSend(client);
}

void CometServer::finalizeAndSend(WiFiClient &client)
{
    m_payload += "]}";
    client.println("Content-Length: " + String(m_payload.length()));
    client.println();
    client.println(m_payload);
    client.flush();

    m_payload = "";
    m_needHandleClient = true;
}
