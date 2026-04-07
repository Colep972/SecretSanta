#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <atomic>
#include <iostream>
#include <fstream>
#include <limits>
#include <map>
#include <string>
#include <windows.h>
#include <shellapi.h>
#include <urlmon.h>
#pragma comment(lib, "urlmon.lib")
#include <ixwebsocket/IXWebSocket.h>
#include "NetworkUtils.h"
#include "../external/json.hpp"

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Input helpers
// ---------------------------------------------------------------------------

static int readInt(const std::string& prompt)
{
    int value;
    while (true)
    {
        std::cout << prompt;
        if (std::cin >> value)
        {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return value;
        }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "  Entree invalide, veuillez entrer un nombre.\n";
    }
}

static std::string readString(const std::string& prompt)
{
    std::string value;
    while (true)
    {
        std::cout << prompt;
        std::getline(std::cin, value);
        if (!value.empty()) return value;
        std::cout << "  Entree invalide, veuillez entrer une valeur.\n";
    }
}

static std::string readOptionalString(const std::string& prompt)
{
    std::cout << prompt;
    std::string value;
    std::getline(std::cin, value);
    return value;
}

static const std::string ADMIN_TOKEN = "GENIE-ADMIN-I-KNOW-COLEP-972-/-MATHIEU";
static const std::string APP_VERSION = "1.0.2";

// ---------------------------------------------------------------------------
// Auto-update
// ---------------------------------------------------------------------------

static void checkForUpdate()
{
    try
    {
        // Check version from server
        json req;
        req["action"] = "GET_VERSION";
        json res = sendRequest(req);

        if (res["status"] != "OK") return;
        std::string serverVersion = res.value("version", "");
        if (serverVersion.empty() || serverVersion == APP_VERSION) return;

        std::cout << "  Mise a jour disponible (" << serverVersion << "), telechargement...\n";

        // Download new exe using Windows URLDownloadToFile
        std::wstring url = L"https://santa.colep.fr/download";
        HRESULT hr = URLDownloadToFileW(NULL, url.c_str(), L"Client_new.exe", 0, NULL);
        if (FAILED(hr)) return;

        // Write updater batch script
        std::ofstream bat("updater.bat");
        bat << "@echo off\r\n";
        bat << "timeout /t 2 /nobreak > nul\r\n";
        bat << "move /y Client_new.exe Client.exe\r\n";
        bat << "start Client.exe\r\n";
        bat << "del updater.bat\r\n";
        bat.close();

        // Launch updater and exit
        ShellExecuteA(NULL, "open", "updater.bat", NULL, NULL, SW_HIDE);
        exit(0);
    }
    catch (...) {}
}

// Session state
static std::string m_profileToken;
static std::string m_name;
static std::string m_email;
static std::string m_inviteCode;
static std::string m_crewToken;
static bool        m_isAdmin = false;
static int         m_participantCount = 0;
static bool        m_drawDone = false;

// ---------------------------------------------------------------------------

static void refreshCrewStatus()
{
    if (m_inviteCode.empty()) return;
    try {
        json req;
        req["action"] = "GET_CREW_STATUS";
        req["invite_code"] = m_inviteCode;
        json res = sendRequest(req);
        if (res["status"] == "OK") {
            m_participantCount = res["participants_count"].get<int>();
            m_drawDone = res["draw_done"].get<bool>();
        }
    }
    catch (...) {}
}

static void printResponse(const json& res)
{
    std::string status = res.value("status", "ERROR");
    std::cout << (status == "OK" ? "[OK]" : "[ERREUR]") << "\n";

    if (res.contains("message"))
        std::cout << "  " << res["message"].get<std::string>() << "\n";
    if (res.contains("invite_code"))
        std::cout << "  Code crew    : " << res["invite_code"].get<std::string>() << "\n";
    if (res.contains("participants_count"))
        std::cout << "  Participants : " << res["participants_count"] << "\n";
    if (res.contains("name"))
        std::cout << "  Nom          : " << res["name"].get<std::string>() << "\n";
    if (res.contains("email"))
        std::cout << "  Email        : " << res["email"].get<std::string>() << "\n";
    if (res.contains("participants")) {
        std::cout << "  Participants :\n";
        for (const auto& p : res["participants"])
            std::cout << "    - " << p.get<std::string>() << "\n";
    }
    if (res.contains("wishes")) {
        auto& wishes = res["wishes"];
        if (wishes.empty()) {
            std::cout << "  Liste de voeux : (vide)\n";
        }
        else {
            std::cout << "  Liste de voeux :\n";
            int idx = 0;
            for (const auto& w : wishes)
                std::cout << "    [" << idx++ << "] " << w.get<std::string>() << "\n";
        }
    }
    if (res.contains("crews")) {
        auto& crews = res["crews"];
        if (crews.empty()) {
            std::cout << "  Crews : (aucun)\n";
        }
        else {
            std::cout << "  Crews :\n";
            for (const auto& c : crews)
                std::cout << "    - " << c["code"].get<std::string>() << "\n";
        }
    }
}

static json call(const json& req)
{
    try {
        return sendRequest(req);
    }
    catch (const std::exception& e) {
        return json{ {"status","ERROR"},{"message", std::string(e.what())} };
    }
}

// ---------------------------------------------------------------------------
// Profile menu
// ---------------------------------------------------------------------------

static void profileMenu()
{
    while (true)
    {
        std::cout << "\n  Bienvenue sur Genie !\n\n";
        std::cout << "  1. Creer un profil\n";
        std::cout << "  2. Se connecter\n";
        std::cout << "  3. Continuer sans profil\n";
        std::cout << "  4. Quitter\n";
        int choice = readInt("  Choix : ");

        if (choice == 4)
        {
            exit(0);
        }
        else if (choice == 1)
        {
            std::string username, password, name, email;
            username = readString("Nom d'utilisateur : ");
            password = readString("Mot de passe      : ");

            if (!m_name.empty()) {
                std::string input = readOptionalString("Votre nom [" + m_name + "] (Entree pour confirmer) : ");
                name = input.empty() ? m_name : input;
            }
            else {
                name = readString("Votre nom   : ");
            }

            if (!m_email.empty()) {
                std::string input = readOptionalString("Votre email [" + m_email + "] (Entree pour confirmer) : ");
                email = input.empty() ? m_email : input;
            }
            else {
                email = readString("Votre email : ");
            }

            json req;
            req["action"] = "CREATE_PROFILE";
            req["username"] = username;
            req["password"] = password;
            req["name"] = name;
            req["email"] = email;

            json res = call(req);
            printResponse(res);

            if (res["status"] == "OK") {
                m_profileToken = res["profile_token"].get<std::string>();
                m_name = res["name"].get<std::string>();
                m_email = res["email"].get<std::string>();

                if (!m_inviteCode.empty() && !m_crewToken.empty()) {
                    json linkReq;
                    linkReq["action"] = "LINK_CREW_TO_PROFILE";
                    linkReq["profile_token"] = m_profileToken;
                    linkReq["invite_code"] = m_inviteCode;
                    linkReq["crew_token"] = m_crewToken;
                    call(linkReq);
                    std::cout << "  Crew " << m_inviteCode << " lie a votre profil.\n";
                }
                return;
            }
        }
        else if (choice == 2)
        {
            std::string username, password;
            username = readString("Nom d'utilisateur : ");
            password = readString("Mot de passe      : ");

            json req;
            req["action"] = "LOGIN_PROFILE";
            req["username"] = username;
            req["password"] = password;

            json res = call(req);
            printResponse(res);

            if (res["status"] == "OK") {
                m_profileToken = res["profile_token"].get<std::string>();
                m_name = res["name"].get<std::string>();
                m_email = res["email"].get<std::string>();

                if (res.contains("crews") && !res["crews"].empty()) {
                    auto& firstCrew = res["crews"][0];
                    m_inviteCode = firstCrew["code"].get<std::string>();
                    m_crewToken = firstCrew["token"].get<std::string>();
                    refreshCrewStatus();
                    std::cout << "  Crew restaure : " << m_inviteCode << "\n";
                }
                return;
            }
        }
        else if (choice == 3)
        {
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// Main menu
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "Genie - santa.colep.fr\n";

    checkForUpdate();

    profileMenu();

    bool running = true;
    while (running)
    {
        std::cout << "\n";

        if (!m_name.empty())
            std::cout << "  [" << m_name
            << (!m_inviteCode.empty() ? " | crew " + m_inviteCode : "")
            << (m_isAdmin ? " | admin" : "")
            << "]\n\n";

        int opt = 1;
        std::map<int, std::string> menu;

        if (m_profileToken.empty())
            menu[opt++] = "CREATE_PROFILE_MAIN";
        menu[opt++] = "CREATE_CREW";
        menu[opt++] = "LOGIN_CREW";
        menu[opt++] = "JOIN_CREW";

        if (m_isAdmin && m_participantCount >= 3 && !m_drawDone)
            menu[opt++] = "RUN_DRAW";
        if (m_isAdmin && m_drawDone)
            menu[opt++] = "SEND_EMAILS";
        if (!m_profileToken.empty()) {
            menu[opt++] = "GET_WISHES";
            menu[opt++] = "ADD_WISH";
            menu[opt++] = "EXPORT_WISHLIST";
            menu[opt++] = "REMOVE_WISH";
            menu[opt++] = "GET_PROFILE";
        }
        if (!m_inviteCode.empty()) {
            menu[opt++] = "GET_PARTICIPANTS";
            menu[opt++] = "CHAT";
            menu[opt++] = "GET_POTS";
            menu[opt++] = "MARK_PAID";
        }
        if (m_isAdmin) {
            menu[opt++] = "CREATE_POT";
            menu[opt++] = "REMOVE_PARTICIPANT";
            if (m_drawDone)
                menu[opt++] = "RESET_DRAW";
        }
        menu[opt++] = "QUIT";

        static const std::map<std::string, std::string> labels = {
            {"CREATE_CREW",         "Creer un crew"},
            {"LOGIN_CREW",          "Se connecter a un crew"},
            {"JOIN_CREW",           "Rejoindre un crew"},
            {"RUN_DRAW",            "Lancer le tirage"},
            {"SEND_EMAILS",         "Envoyer les emails"},
            {"GET_WISHES",          "Voir ma liste de voeux"},
            {"EXPORT_WISHLIST",     "Exporter ma liste de voeux (HTML)"},
            {"ADD_WISH",            "Ajouter un voeu"},
            {"REMOVE_WISH",         "Supprimer un voeu"},
            {"GET_PROFILE",         "Voir mon profil"},
            {"REMOVE_PARTICIPANT",  "Retirer un participant"},
            {"CREATE_POT",          "Creer un pot"},
            {"GET_PARTICIPANTS",    "Voir les participants"},
            {"CHAT",                "Chat du crew"},
            {"GET_POTS",            "Voir les pots"},
            {"MARK_PAID",           "Marquer comme paye"},
            {"RESET_DRAW",          "Reinitialiser le tirage"},
            {"QUIT",                "Quitter"},
            {"CREATE_PROFILE_MAIN", "Creer / Se connecter a un profil"},
        };

        for (const auto& entry : menu)
            std::cout << "  " << entry.first << ". " << labels.at(entry.second) << "\n";

        int choice = readInt("  Choix : ");

        if (menu.find(choice) == menu.end()) continue;
        std::string action = menu[choice];

        // ── CREATE PROFILE FROM MAIN MENU ────────────────────────────────────
        if (action == "CREATE_PROFILE_MAIN")
        {
            profileMenu();
        }

        // ── QUIT ─────────────────────────────────────────────────────────────
        else if (action == "QUIT")
        {
            running = false;
        }

        // ── CREATE CREW ──────────────────────────────────────────────────────
        else if (action == "CREATE_CREW")
        {
            std::string crewName = readString("Nom du crew : ");

            if (m_profileToken.empty() && m_name.empty()) {
                m_name = readString("Votre nom   : ");
                m_email = readString("Votre email : ");
            }

            json req;
            req["action"] = "CREATE_CREW";
            req["crew_name"] = crewName;
            if (!m_profileToken.empty())
                req["profile_token"] = m_profileToken;
            else
                req["user"] = { {"name", m_name}, {"email", m_email} };

            json res = call(req);
            printResponse(res);

            if (res["status"] == "OK") {
                m_inviteCode = res["invite_code"].get<std::string>();
                m_crewToken = res["token"].get<std::string>();
                m_isAdmin = true;
                refreshCrewStatus();
            }
        }

        // ── LOGIN CREW ───────────────────────────────────────────────────────
        else if (action == "LOGIN_CREW")
        {
            std::string code = readString("Code du crew : ");

            if (m_profileToken.empty() && m_name.empty())
                m_name = readString("Votre nom   : ");

            json req;
            req["action"] = "LOGIN_CREW";
            req["invite_code"] = code;
            if (!m_profileToken.empty())
                req["profile_token"] = m_profileToken;
            else
                req["name"] = m_name;

            json res = call(req);
            printResponse(res);

            if (res["status"] == "OK") {
                m_inviteCode = code;
                m_crewToken = res["token"].get<std::string>();
                m_isAdmin = res["is_owner"].get<bool>();
                refreshCrewStatus();
            }
        }

        // ── JOIN CREW ────────────────────────────────────────────────────────
        else if (action == "JOIN_CREW")
        {
            std::string code = readString("Code du crew : ");

            if (m_profileToken.empty() && m_name.empty()) {
                m_name = readString("Votre nom   : ");
                m_email = readString("Votre email : ");
            }

            json req;
            req["action"] = "JOIN_CREW";
            req["invite_code"] = code;
            if (!m_profileToken.empty())
                req["profile_token"] = m_profileToken;
            else
                req["user"] = { {"name", m_name}, {"email", m_email} };

            json res = call(req);
            printResponse(res);

            if (res["status"] == "OK") {
                m_inviteCode = code;
                m_crewToken = res["token"].get<std::string>();
                m_isAdmin = false;
                refreshCrewStatus();
            }
        }

        // ── RUN DRAW ─────────────────────────────────────────────────────────
        else if (action == "RUN_DRAW")
        {
            json req;
            req["action"] = "RUN_DRAW";
            req["invite_code"] = m_inviteCode;
            req["admin_token"] = ADMIN_TOKEN;
            json res = call(req);
            printResponse(res);
            if (res["status"] == "OK") refreshCrewStatus();
        }

        // ── SEND EMAILS ──────────────────────────────────────────────────────
        else if (action == "SEND_EMAILS")
        {
            json req;
            req["action"] = "SEND_EMAILS";
            req["invite_code"] = m_inviteCode;
            req["admin_token"] = ADMIN_TOKEN;
            printResponse(call(req));
        }

        // ── EXPORT WISHLIST ──────────────────────────────────────────────────
        else if (action == "EXPORT_WISHLIST")
        {
            json req;
            req["action"] = "GET_WISHES";
            req["profile_token"] = m_profileToken;
            json res = call(req);

            if (res["status"] != "OK") { printResponse(res); }
            else
            {
                std::string name = res.value("name", m_name);
                auto& wishes = res["wishes"];

                std::string html =
                    "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
                    "<title>Liste de voeux - " + name + "</title>"
                    "<style>"
                    "body{font-family:Georgia,serif;max-width:600px;margin:60px auto;color:#222;}"
                    "h1{color:#c0392b;border-bottom:2px solid #c0392b;padding-bottom:10px;}"
                    "h2{color:#555;font-size:1rem;margin-top:-10px;margin-bottom:30px;}"
                    "ul{list-style:none;padding:0;}"
                    "li{padding:10px 15px;margin:8px 0;background:#fdf6f0;border-left:4px solid #c0392b;border-radius:4px;font-size:1.05rem;}"
                    ".empty{color:#999;font-style:italic;}"
                    "@media print{body{margin:20px;}}"
                    "</style></head><body>"
                    "<h1>&#127873; Liste de voeux</h1>"
                    "<h2>" + name + "</h2><ul>";

                if (wishes.empty())
                    html += "<li class='empty'>Aucun voeu pour l'instant.</li>";
                else
                    for (const auto& w : wishes)
                        html += "<li>" + w.get<std::string>() + "</li>";

                html += "</ul></body></html>";

                std::string filename = "wishlist_" + name + ".html";
                std::ofstream f(filename);
                if (f.is_open()) {
                    f << html;
                    f.close();
                    std::cout << "[OK] Fichier genere : " << filename << "\n";
                    ShellExecuteA(NULL, "open", filename.c_str(), NULL, NULL, SW_SHOWNORMAL);
                }
                else {
                    std::cout << "[ERREUR] Impossible de creer le fichier.\n";
                }
            }
        }

        // ── GET WISHES ───────────────────────────────────────────────────────
        else if (action == "GET_WISHES")
        {
            json req;
            req["action"] = "GET_WISHES";
            req["profile_token"] = m_profileToken;
            printResponse(call(req));
        }

        // ── ADD WISH ─────────────────────────────────────────────────────────
        else if (action == "ADD_WISH")
        {
            std::string wish = readString("Votre voeu : ");

            json req;
            req["action"] = "ADD_WISH";
            req["profile_token"] = m_profileToken;
            req["wish"] = wish;
            printResponse(call(req));
        }

        // ── REMOVE WISH ──────────────────────────────────────────────────────
        else if (action == "REMOVE_WISH")
        {
            json getReq;
            getReq["action"] = "GET_WISHES";
            getReq["profile_token"] = m_profileToken;
            json getRes = call(getReq);
            printResponse(getRes);

            if (!getRes.contains("wishes") || getRes["wishes"].empty()) {
                std::cout << "  Aucun voeu a supprimer.\n"; continue;
            }

            int index = readInt("Index a supprimer : ");

            json req;
            req["action"] = "REMOVE_WISH";
            req["profile_token"] = m_profileToken;
            req["index"] = index;
            printResponse(call(req));
        }

        // ── GET PROFILE ──────────────────────────────────────────────────────
        else if (action == "GET_PROFILE")
        {
            json req;
            req["action"] = "GET_PROFILE";
            req["profile_token"] = m_profileToken;
            printResponse(call(req));
        }

        // ── CHAT ─────────────────────────────────────────────────────────────
        else if (action == "CHAT")
        {
            std::string name = m_name.empty() ? "Inconnu" : m_name;

            ix::WebSocket ws;
            std::string url = "wss://chat.colep.fr";
            ws.setUrl(url);

            std::atomic<bool> chatRunning(true);

            ws.setOnMessageCallback([&](const ix::WebSocketMessagePtr& msg)
                {
                    if (msg->type == ix::WebSocketMessageType::Open)
                    {
                        // Send join message
                        json join;
                        join["type"] = "join";
                        join["crew_code"] = m_inviteCode;
                        join["name"] = name;
                        ws.send(join.dump());
                    }
                    else if (msg->type == ix::WebSocketMessageType::Message)
                    {
                        try
                        {
                            json j = json::parse(msg->str);
                            std::string type = j.value("type", "");
                            if (type == "message")
                                std::cout << "\n  [" << j["timestamp"].get<std::string>() << "] "
                                << j["sender"].get<std::string>() << ": "
                                << j["text"].get<std::string>() << "\n  > ";
                            else if (type == "system")
                                std::cout << "\n  ** " << j["text"].get<std::string>() << " **\n  > ";
                            std::cout.flush();
                        }
                        catch (...) {}
                    }
                    else if (msg->type == ix::WebSocketMessageType::Close ||
                        msg->type == ix::WebSocketMessageType::Error)
                    {
                        chatRunning = false;
                    }
                });

            ws.start();

            std::cout << "  Chat du crew " << m_inviteCode << " (tapez /quit pour quitter)\n";

            while (chatRunning)
            {
                std::cout << "  > ";
                std::string line;
                std::getline(std::cin, line);
                if (line == "/quit") break;
                if (line.empty()) continue;

                json msg;
                msg["type"] = "message";
                msg["text"] = line;
                ws.send(msg.dump());
            }

            ws.stop();
            std::cout << "  Chat ferme.\n";
        }

        // ── GET PARTICIPANTS ─────────────────────────────────────────────────
        else if (action == "GET_PARTICIPANTS")
        {
            json req;
            req["action"] = "GET_PARTICIPANTS";
            req["invite_code"] = m_inviteCode;
            printResponse(call(req));
        }

        // ── GET POTS ─────────────────────────────────────────────────────────
        else if (action == "GET_POTS")
        {
            json req;
            req["action"] = "GET_POTS";
            req["invite_code"] = m_inviteCode;
            json res = call(req);

            if (res["status"] != "OK") { printResponse(res); }
            else
            {
                auto& pots = res["pots"];
                if (pots.empty()) {
                    std::cout << "  Aucun pot pour ce crew.\n";
                }
                else {
                    for (const auto& pot : pots)
                    {
                        std::cout << "\n  [Pot] " << pot["name"].get<std::string>()
                            << " - " << pot["amount"].get<double>() << " EUR/pers"
                            << " (cree par " << pot["creator"].get<std::string>() << ")\n";
                        std::cout << "  ID: " << pot["id"].get<std::string>() << "\n";
                        for (const auto& p : pot["participants"])
                            std::cout << "    " << (p["paid"].get<bool>() ? "[X] " : "[ ] ")
                            << p["name"].get<std::string>() << "\n";
                    }
                }
            }
        }

        // ── CREATE POT ───────────────────────────────────────────────────────
        else if (action == "CREATE_POT")
        {
            std::string potName = readString("Nom du pot : ");
            std::string amountStr = readString("Montant par personne (EUR) : ");
            double amount = std::stod(amountStr);

            json req;
            req["action"] = "CREATE_POT";
            req["invite_code"] = m_inviteCode;
            req["admin_token"] = ADMIN_TOKEN;
            req["pot_name"] = potName;
            req["amount"] = amount;
            if (!m_profileToken.empty())
                req["profile_token"] = m_profileToken;
            else
                req["creator"] = m_name;
            printResponse(call(req));
        }

        // ── MARK PAID ────────────────────────────────────────────────────────
        else if (action == "MARK_PAID")
        {
            json getReq;
            getReq["action"] = "GET_POTS";
            getReq["invite_code"] = m_inviteCode;
            json getRes = call(getReq);

            if (getRes["status"] != "OK" || getRes["pots"].empty()) {
                std::cout << "  Aucun pot disponible.\n"; continue;
            }

            for (const auto& pot : getRes["pots"])
                std::cout << "  " << pot["id"].get<std::string>().substr(0, 8)
                << "... - " << pot["name"].get<std::string>() << "\n";

            std::string potId = readString("ID du pot (premiers caracteres) : ");

            std::string fullId;
            for (const auto& pot : getRes["pots"])
            {
                std::string id = pot["id"].get<std::string>();
                if (id.substr(0, potId.size()) == potId) { fullId = id; break; }
            }
            if (fullId.empty()) { std::cout << "  Pot introuvable.\n"; continue; }

            json req;
            req["action"] = "MARK_PAID";
            req["invite_code"] = m_inviteCode;
            req["pot_id"] = fullId;
            if (!m_profileToken.empty())
                req["profile_token"] = m_profileToken;
            else
                req["name"] = m_name;
            printResponse(call(req));
        }

        // ── REMOVE PARTICIPANT ───────────────────────────────────────────────
        else if (action == "REMOVE_PARTICIPANT")
        {
            json listReq;
            listReq["action"] = "GET_PARTICIPANTS";
            listReq["invite_code"] = m_inviteCode;
            printResponse(call(listReq));

            std::string name = readString("Nom du participant a retirer : ");

            json req;
            req["action"] = "REMOVE_PARTICIPANT";
            req["invite_code"] = m_inviteCode;
            req["admin_token"] = ADMIN_TOKEN;
            req["name"] = name;

            json res = call(req);
            printResponse(res);
            if (res["status"] == "OK") refreshCrewStatus();
        }

        // ── RESET DRAW ───────────────────────────────────────────────────────
        else if (action == "RESET_DRAW")
        {
            char c = readString("  Attention : les emails ont peut-etre deja ete envoyes. Continuer ? (o/n) : ")[0];
            if (c != 'o' && c != 'O') continue;

            json req;
            req["action"] = "RESET_DRAW";
            req["invite_code"] = m_inviteCode;
            req["admin_token"] = ADMIN_TOKEN;

            json res = call(req);
            printResponse(res);
            if (res["status"] == "OK") refreshCrewStatus();
        }
    }

    return 0;
}