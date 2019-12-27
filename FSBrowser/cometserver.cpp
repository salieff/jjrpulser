#include "cometserver.h"

CometServer::CometServer(int port)
    : m_server(port)
{
}

void CometServer::setup()
{
    m_server.on(...);
    m_server.begin();
}

void CometServer::work()
{
    m_server.handleClient();
}
