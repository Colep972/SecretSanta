#include "NetworkUtils.h"

bool sendJson(SocketType sock, const std::string& json)
{
    std::string msg = json + "\n";
    int result = send(sock, msg.c_str(), msg.size(), 0);
    return result >= 0;
}

bool recvJson(SocketType sock, std::string& json)
{
    char c;
    json.clear();

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
