#define NOMINMAX   // prevent Windows.h from defining min/max macros
#include <limits>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "NetworkUtils.h"
#include "../external/json.hpp"

#pragma comment(lib, "ws2_32.lib")

using json = nlohmann::json;

// ── Session state ─────────────────────────────────────────────────────────────
static std::string m_inviteCode;
static std::string m_token;
static std::string m_name;
static bool        m_isAdmin = false;
static int         m_participantCount = 0;
static bool        m_drawDone = false;
// ─────────────────────────────────────────────────────────────────────────────

// Fetch participant count and draw status from server, update globals
static void refreshCrewStatus(SOCKET sock)
{
    if (m_inviteCode.empty()) return;

    json req;
    req["action"] = "GET_CREW_STATUS";
    req["invite_code"] = m_inviteCode;
    sendJson(sock, req.dump());

    std::string resp;
    recvJson(sock, resp);

    try
    {
        json res = json::parse(resp);
        if (res["status"] == "OK")
        {
            m_participantCount = res["participants_count"].get<int>();
            m_drawDone = res["draw_done"].get<bool>();
        }
    }
    catch (...) {}
}

void printServerResponse(const std::string& resp)
{
    json res;
    try { res = json::parse(resp); }
    catch (...) { std::cout << "[ERREUR] Reponse non-JSON : " << resp << "\n"; return; }

    if (!res.contains("status"))
    {
        std::cout << "[ERREUR] Reponse invalide\n";
        return;
    }

    std::string status = res["status"].get<std::string>();
    std::cout << (status == "OK" ? "[OK]" : "[ERREUR]") << "\n";

    if (res.contains("message"))
        std::cout << "  " << res["message"].get<std::string>() << "\n";

    if (res.contains("invite_code"))
        std::cout << "  Code crew    : " << res["invite_code"].get<std::string>() << "\n";

    // Token is intentionally NOT displayed here

    if (res.contains("participants_count"))
        std::cout << "  Participants : " << res["participants_count"] << "\n";

    if (res.contains("name"))
        std::cout << "  Profil de    : " << res["name"].get<std::string>() << "\n";

    if (res.contains("participants"))
    {
        auto& parts = res["participants"];
        std::cout << "  Participants :\n";
        int idx = 1;
        for (const auto& p : parts)
            std::cout << "    " << idx++ << ". " << p.get<std::string>() << "\n";
    }

    if (res.contains("wishes"))
    {
        auto& wishes = res["wishes"];
        if (wishes.empty())
        {
            std::cout << "  Liste de voeux : (vide)\n";
        }
        else
        {
            std::cout << "  Liste de voeux :\n";
            int idx = 0;
            for (const auto& w : wishes)
                std::cout << "    [" << idx++ << "] " << w.get<std::string>() << "\n";
        }
    }
}

static bool requireSession()
{
    if (!m_inviteCode.empty() && !m_token.empty())
        return true;

    std::cout << "  [!] Vous devez d'abord creer ou rejoindre un crew.\n";
    return false;
}

int main()
{
    WSADATA wsData;
    WSAStartup(MAKEWORD(2, 2), &wsData);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(54000);
    InetPton(AF_INET, L"100.114.253.106", &server.sin_addr);

    if (connect(sock, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
    {
        std::cerr << "Connexion impossible\n";
        return -1;
    }

    std::cout << "Connexion au serveur etablie !\n";

    bool running = true;
    while (running)
    {
        std::cout << "\n";

        if (!m_inviteCode.empty())
            std::cout << "  [Session : " << m_name << " | crew " << m_inviteCode << "]\n\n";

        std::cout << "  1. Creer un crew\n";
        std::cout << "  2. Se connecter a un crew\n";
        std::cout << "  3. Rejoindre un crew\n";

        if (m_isAdmin)
        {
            std::cout << "  -----------------------------\n";
            if (m_participantCount >= 3)
                std::cout << "  4. Lancer le tirage (admin)\n";
            if (m_drawDone)
                std::cout << "  5. Envoyer les emails (admin)\n";
            std::cout << "  10. Retirer un participant (admin)\n";
            if (m_drawDone)
                std::cout << "  11. Reinitialiser le tirage (admin)\n";
        }

        if (!m_inviteCode.empty())
        {
            std::cout << "  -----------------------------\n";
            std::cout << "  6. Voir ma liste de voeux\n";
            std::cout << "  7. Ajouter un voeu\n";
            std::cout << "  8. Supprimer un voeu\n";
        }

        std::cout << "  -----------------------------\n";
        std::cout << "  9. Quitter\n";
        std::cout << "  Choix : ";

        int choice;
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        // ── 1. CREATE CREW ────────────────────────────────────────────────────
        if (choice == 1)
        {
            std::string crewName, name, email;

            std::cout << "Nom du crew : ";
            std::getline(std::cin, crewName);

            std::cout << "Ton nom     : ";
            std::getline(std::cin, name);

            std::cout << "Ton email   : ";
            std::getline(std::cin, email);

            json req;
            req["action"] = "CREATE_CREW";
            req["crew_name"] = crewName;
            req["user"]["name"] = name;
            req["user"]["email"] = email;

            sendJson(sock, req.dump());

            std::string resp;
            recvJson(sock, resp);
            printServerResponse(resp);

            json res = json::parse(resp);
            if (res["status"] == "OK")
            {
                m_inviteCode = res["invite_code"].get<std::string>();
                m_token = res["token"].get<std::string>();
                m_name = name;
                m_isAdmin = true; // creator is always owner
                std::cout << "  Votre token (notez-le) : " << m_token << "\n";
                refreshCrewStatus(sock);
            }
        }
        // ── 2. LOGIN CREW (already a member, reconnect by token) ──────────────
        else if (choice == 2)
        {
            std::string code, token;

            std::cout << "Code du crew : ";
            std::getline(std::cin, code);

            std::cout << "Votre token  : ";
            std::getline(std::cin, token);

            // Verify token against server by fetching wishes
            json req;
            req["action"] = "GET_WISHES";
            req["invite_code"] = code;
            req["token"] = token;

            sendJson(sock, req.dump());

            std::string resp;
            recvJson(sock, resp);

            json res = json::parse(resp);
            if (res["status"] == "OK")
            {
                m_inviteCode = code;
                m_token = token;
                m_name = res["name"].get<std::string>();
                m_isAdmin = res.value("is_owner", false);
                std::cout << "[OK]\n  Session restauree ! Bienvenue " << m_name << "\n";
                if (m_isAdmin) std::cout << "  Mode admin active.\n";
                refreshCrewStatus(sock);
            }
            else
            {
                std::cout << "[ERREUR]\n  Token ou code invalide.\n";
            }
        }
        // ── 3. JOIN CREW (new member) ─────────────────────────────────────────
        else if (choice == 3)
        {
            std::string code, name, email;

            std::cout << "Code du crew : ";
            std::getline(std::cin, code);

            std::cout << "Ton nom      : ";
            std::getline(std::cin, name);

            std::cout << "Ton email    : ";
            std::getline(std::cin, email);

            json req;
            req["action"] = "JOIN_CREW";
            req["invite_code"] = code;
            req["user"]["name"] = name;
            req["user"]["email"] = email;

            sendJson(sock, req.dump());

            std::string resp;
            recvJson(sock, resp);
            printServerResponse(resp);

            json res = json::parse(resp);
            if (res["status"] == "OK")
            {
                m_inviteCode = code;
                m_token = res["token"].get<std::string>();
                m_name = name;
                m_isAdmin = res.value("is_owner", false);
                std::cout << "  Votre token (notez-le) : " << m_token << "\n";
                refreshCrewStatus(sock);
            }
        }
        // ── 4. RUN DRAW (admin only) ──────────────────────────────────────────
        else if (choice == 4)
        {
            if (!m_isAdmin) { std::cout << "  [!] Acces refuse.\n"; continue; }

            std::string code;
            std::cout << "Code du crew : ";
            std::getline(std::cin, code);

            json req;
            req["action"] = "RUN_DRAW";
            req["invite_code"] = code;
            req["admin_token"] = "SANTA-ADMIN-I-KNOW-COLEP-972-/-MATHIEU";

            sendJson(sock, req.dump());

            std::string resp;
            recvJson(sock, resp);
            printServerResponse(resp);

            // Unlock "send emails" option if draw succeeded
            json res = json::parse(resp);
            if (res["status"] == "OK")
                refreshCrewStatus(sock);
        }
        // ── 5. SEND EMAILS (admin only) ───────────────────────────────────────
        else if (choice == 5)
        {
            if (!m_isAdmin) { std::cout << "  [!] Acces refuse.\n"; continue; }

            std::string code;
            std::cout << "Code du crew : ";
            std::getline(std::cin, code);

            json req;
            req["action"] = "SEND_EMAILS";
            req["invite_code"] = code;

            sendJson(sock, req.dump());

            std::string resp;
            recvJson(sock, resp);
            printServerResponse(resp);
        }
        // ── 6. GET WISHES ─────────────────────────────────────────────────────
        else if (choice == 6)
        {
            if (!requireSession()) continue;

            json req;
            req["action"] = "GET_WISHES";
            req["invite_code"] = m_inviteCode;
            req["token"] = m_token;

            sendJson(sock, req.dump());

            std::string resp;
            recvJson(sock, resp);
            printServerResponse(resp);
        }
        // ── 7. ADD WISH ───────────────────────────────────────────────────────
        else if (choice == 7)
        {
            if (!requireSession()) continue;

            std::string wish;
            std::cout << "Votre voeu : ";
            std::getline(std::cin, wish);

            json req;
            req["action"] = "ADD_WISH";
            req["invite_code"] = m_inviteCode;
            req["token"] = m_token;
            req["wish"] = wish;

            sendJson(sock, req.dump());

            std::string resp;
            recvJson(sock, resp);
            printServerResponse(resp);
        }
        // ── 8. REMOVE WISH ────────────────────────────────────────────────────
        else if (choice == 8)
        {
            if (!requireSession()) continue;

            // Show current wishes first so user knows the indices
            {
                json req;
                req["action"] = "GET_WISHES";
                req["invite_code"] = m_inviteCode;
                req["token"] = m_token;
                sendJson(sock, req.dump());
                std::string resp;
                recvJson(sock, resp);
                printServerResponse(resp);

                json res = json::parse(resp);
                if (!res.contains("wishes") || res["wishes"].empty())
                {
                    std::cout << "  Aucun voeu a supprimer.\n";
                    continue;
                }
            }

            int index;
            std::cout << "Index a supprimer : ";
            std::cin >> index;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            json req;
            req["action"] = "REMOVE_WISH";
            req["invite_code"] = m_inviteCode;
            req["token"] = m_token;
            req["index"] = index;

            sendJson(sock, req.dump());

            std::string resp;
            recvJson(sock, resp);
            printServerResponse(resp);
        }
        // ── 10. REMOVE PARTICIPANT (admin only) ───────────────────────────────
        else if (choice == 10)
        {
            if (!m_isAdmin) { std::cout << "  [!] Acces refuse.\n"; continue; }
            if (!requireSession()) continue;

            // Fetch and display participant list first
            {
                json req;
                req["action"] = "GET_PARTICIPANTS";
                req["invite_code"] = m_inviteCode;
                sendJson(sock, req.dump());
                std::string resp;
                recvJson(sock, resp);
                printServerResponse(resp);

                json res = json::parse(resp);
                if (!res.contains("participants") || res["participants"].empty())
                {
                    std::cout << "  Aucun participant.\n";
                    continue;
                }
            }

            std::string name;
            std::cout << "Nom du participant a retirer : ";
            std::getline(std::cin, name);

            json req;
            req["action"] = "REMOVE_PARTICIPANT";
            req["invite_code"] = m_inviteCode;
            req["admin_token"] = "SANTA-ADMIN-I-KNOW-COLEP-972-/-MATHIEU";
            req["name"] = name;

            sendJson(sock, req.dump());

            std::string resp;
            recvJson(sock, resp);
            printServerResponse(resp);

            json res = json::parse(resp);
            if (res["status"] == "OK")
            {
                m_drawDone = false; // draw invalidated by removal
                refreshCrewStatus(sock);
            }
        }
        // ── 11. RESET DRAW (admin only) ───────────────────────────────────────
        else if (choice == 11)
        {
            if (!m_isAdmin) { std::cout << "  [!] Acces refuse.\n"; continue; }
            if (!requireSession()) continue;

            std::cout << "  /!\\ ATTENTION : Les emails ont peut-etre deja ete envoyes.\n";
            std::cout << "  Confirmer la reinitialisation du tirage ? (o/n) : ";
            std::string confirm;
            std::getline(std::cin, confirm);
            if (confirm != "o" && confirm != "O")
            {
                std::cout << "  Annule.\n";
                continue;
            }

            json req;
            req["action"] = "RESET_DRAW";
            req["invite_code"] = m_inviteCode;
            req["admin_token"] = "SANTA-ADMIN-I-KNOW-COLEP-972-/-MATHIEU";

            sendJson(sock, req.dump());

            std::string resp;
            recvJson(sock, resp);
            printServerResponse(resp);

            json res = json::parse(resp);
            if (res["status"] == "OK")
                refreshCrewStatus(sock);
        }
        // ── 9. QUIT ───────────────────────────────────────────────────────────
        else if (choice == 9)
        {
            running = false;
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}