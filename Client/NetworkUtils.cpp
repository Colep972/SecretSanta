#include "NetworkUtils.h"

bool sendJson(SOCKET sock, const std::string& json)
{
    std::string msg = json + "\n";
    return send(sock, msg.c_str(), msg.size(), 0) != SOCKET_ERROR;
}

bool recvJson(SOCKET sock, std::string& json)
{
    json.clear();
    char c;

    while (true)
    {
        int bytes = recv(sock, &c, 1, 0);
        if (bytes <= 0)
            return false;

        if (c == '\n')
            break;

        json += c;
    }
    return true;
}
