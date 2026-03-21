#pragma once
#include <string>
#include <vector>
#include <map>
#include <set>
#include <mutex>
#include <thread>
#include <functional>

struct ChatMessage
{
    std::string sender;
    std::string text;
    std::string timestamp;
    std::string crewCode;
};

class ChatServer
{
public:
    ChatServer(int port, const std::string& dataDir);
    ~ChatServer();

    void start();
    void stop();

    // Called by lws callback
    void onConnect(void* wsi, const std::string& crewCode, const std::string& name);
    void onDisconnect(void* wsi);
    void onMessage(void* wsi, const std::string& message);
    void broadcast(const std::string& crewCode, const std::string& message, void* exclude = nullptr);

    static ChatServer* instance;

private:
    int m_port;
    std::string m_dataDir;
    std::thread m_thread;
    bool m_running = false;
    struct lws_context* m_context = nullptr;

    std::mutex m_mutex;

    // wsi -> {crewCode, name}
    struct ClientInfo { std::string crewCode; std::string name; };
    std::map<void*, ClientInfo> m_clients;

    // wsi -> pending write buffers
    std::map<void*, std::vector<std::string>> m_writeQueues;

    void saveMessage(const ChatMessage& msg);
    std::string currentTimestamp();
};