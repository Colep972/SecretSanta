#include "ChatServer.h"
#include <libwebsockets.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

using json = nlohmann::json;

ChatServer* ChatServer::instance = nullptr;

// ---------------------------------------------------------------------------
// LWS callback
// ---------------------------------------------------------------------------

static int ws_callback(struct lws* wsi, enum lws_callback_reasons reason,
    void* user, void* in, size_t len)
{
    ChatServer* srv = ChatServer::instance;
    if (!srv) return 0;

    switch (reason)
    {
    case LWS_CALLBACK_ESTABLISHED:
        break;

    case LWS_CALLBACK_CLOSED:
        srv->onDisconnect(wsi);
        break;

    case LWS_CALLBACK_RECEIVE:
        if (in && len > 0)
            srv->onMessage(wsi, std::string((char*)in, len));
        break;

    case LWS_CALLBACK_SERVER_WRITEABLE:
    {
        std::lock_guard<std::mutex> lock(srv->m_mutex);
        auto it = srv->m_writeQueues.find(wsi);
        if (it != srv->m_writeQueues.end() && !it->second.empty())
        {
            std::string& msg = it->second.front();
            std::vector<unsigned char> buf(LWS_PRE + msg.size());
            memcpy(buf.data() + LWS_PRE, msg.data(), msg.size());
            lws_write(wsi, buf.data() + LWS_PRE, msg.size(), LWS_WRITE_TEXT);
            it->second.erase(it->second.begin());
            if (!it->second.empty())
                lws_callback_on_writable(wsi);
        }
        break;
    }

    default:
        break;
    }
    return 0;
}

static struct lws_protocols protocols[] = {
    { "genie-chat", ws_callback, 0, 65536, 0, nullptr, 0 },
    { nullptr, nullptr, 0, 0, 0, nullptr, 0 }
};

// ---------------------------------------------------------------------------
// ChatServer implementation
// ---------------------------------------------------------------------------

ChatServer::ChatServer(int port, const std::string& dataDir)
    : m_port(port), m_dataDir(dataDir)
{
    instance = this;
}

ChatServer::~ChatServer()
{
    stop();
}

void ChatServer::start()
{
    m_running = true;
    m_thread = std::thread([this]()
        {
            struct lws_context_creation_info info = {};
            info.port = m_port;
            info.protocols = protocols;

            m_context = lws_create_context(&info);
            if (!m_context) { std::cerr << "Failed to create LWS context\n"; return; }

            std::cout << "Chat WebSocket server on port " << m_port << "...\n";
            while (m_running)
                lws_service(m_context, 50);

            lws_context_destroy(m_context);
        });
}

void ChatServer::stop()
{
    m_running = false;
    if (m_thread.joinable()) m_thread.join();
}

void ChatServer::onConnect(void* wsi, const std::string& crewCode, const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_clients[wsi] = { crewCode, name };
    m_writeQueues[wsi] = {};

    // Send chat history
    std::string chatFile = m_dataDir + crewCode + "_chat.json";
    std::ifstream f(chatFile);
    if (f.is_open())
    {
        json history; f >> history;
        for (const auto& msg : history)
        {
            json out;
            out["type"] = "message";
            out["sender"] = msg["sender"];
            out["text"] = msg["text"];
            out["timestamp"] = msg["timestamp"];
            m_writeQueues[wsi].push_back(out.dump());
        }
        lws_callback_on_writable((struct lws*)wsi);
    }
}

void ChatServer::onDisconnect(void* wsi)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_clients.find(wsi);
    if (it != m_clients.end())
    {
        std::string crewCode = it->second.crewCode;
        std::string name = it->second.name;
        m_clients.erase(it);
        m_writeQueues.erase(wsi);

        json out;
        out["type"] = "system";
        out["text"] = name + " a quitte le chat.";
        broadcast(crewCode, out.dump(), wsi);
    }
}

void ChatServer::onMessage(void* wsi, const std::string& message)
{
    try
    {
        json j = json::parse(message);
        std::string type = j.value("type", "");

        if (type == "join")
        {
            std::string crewCode = j["crew_code"].get<std::string>();
            std::string name = j["name"].get<std::string>();
            onConnect(wsi, crewCode, name);

            json out;
            out["type"] = "system";
            out["text"] = name + " a rejoint le chat.";
            broadcast(crewCode, out.dump(), wsi);
        }
        else if (type == "message")
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_clients.find(wsi);
            if (it == m_clients.end()) return;

            std::string crewCode = it->second.crewCode;
            std::string sender = it->second.name;
            std::string text = j["text"].get<std::string>();
            std::string ts = currentTimestamp();

            ChatMessage msg{ sender, text, ts, crewCode };
            saveMessage(msg);

            json out;
            out["type"] = "message";
            out["sender"] = sender;
            out["text"] = text;
            out["timestamp"] = ts;
            broadcast(crewCode, out.dump());
        }
    }
    catch (...) {}
}

void ChatServer::broadcast(const std::string& crewCode, const std::string& message, void* exclude)
{
    for (auto& [wsi, info] : m_clients)
    {
        if (wsi == exclude) continue;
        if (info.crewCode != crewCode) continue;
        m_writeQueues[wsi].push_back(message);
        lws_callback_on_writable((struct lws*)wsi);
    }
}

void ChatServer::saveMessage(const ChatMessage& msg)
{
    std::string chatFile = m_dataDir + msg.crewCode + "_chat.json";
    json history = json::array();
    std::ifstream in(chatFile);
    if (in.is_open()) { in >> history; in.close(); }

    json m;
    m["sender"] = msg.sender;
    m["text"] = msg.text;
    m["timestamp"] = msg.timestamp;
    history.push_back(m);

    std::ofstream out(chatFile);
    out << history.dump(4);
}

std::string ChatServer::currentTimestamp()
{
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%d/%m %H:%M");
    return oss.str();
}