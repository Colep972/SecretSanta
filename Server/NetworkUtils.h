#pragma once
#include <string>

#ifdef _WIN32
#include <winsock2.h>
using SocketType = SOCKET;
#else
#include <sys/socket.h>
#include <unistd.h>
using SocketType = int;
#endif

bool sendJson(SocketType sock, const std::string& json);
bool recvJson(SocketType sock, std::string& json);