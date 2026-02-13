#pragma once
#include <string>
#include <winsock2.h>

bool sendJson(SOCKET sock, const std::string& json);
bool recvJson(SOCKET sock, std::string& json);

