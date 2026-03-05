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
static std::string g_inviteCode;
static std::string g_token;
static std::string g_name;
static bool        g_isAdmin = false;
// ─────────────────────────────────────────────────────────────────────────────

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
    if (!g_inviteCode.empty() && !g_token.empty())
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

        if (!g_inviteCode.empty())
            std::cout << "  [Session : " << g_name << " | crew " << g_inviteCode << "]\n\n";

        std::cout << "  1. Creer un crew\n";
        std::cout << "  2. Se connecter a un crew\n";
        std::cout << "  3. Rejoindre un crew\n";

        if (g_isAdmin)
        {
            std::cout << "  -----------------------------\n";
            std::cout << "  4. Lancer le tirage (admin)\n";
            std::cout << "  5. Envoyer les emails (admin)\n";
        }

        if (!g_inviteCode.empty())
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
                g_inviteCode = res["invite_code"].get<std::string>();
                g_token = res["token"].get<std::string>();
                g_name = name;
                g_isAdmin = true; // creator is always owner
                std::cout << "  Votre token (notez-le) : " << g_token << "\n";
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
                g_inviteCode = code;
                g_token = token;
                g_name = res["name"].get<std::string>();
                g_isAdmin = res.value("is_owner", false);
                std::cout << "[OK]\n  Session restauree ! Bienvenue " << g_name << "\n";
                if (g_isAdmin) std::cout << "  Mode admin active.\n";
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
                g_inviteCode = code;
                g_token = res["token"].get<std::string>();
                g_name = name;
                g_isAdmin = res.value("is_owner", false);
                std::cout << "  Votre token (notez-le) : " << g_token << "\n";
            }
        }
        // ── 4. RUN DRAW (admin only) ──────────────────────────────────────────
        else if (choice == 4)
        {
            if (!g_isAdmin) { std::cout << "  [!] Acces refuse.\n"; continue; }

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
        }
        // ── 5. SEND EMAILS (admin only) ───────────────────────────────────────
        else if (choice == 5)
        {
            if (!g_isAdmin) { std::cout << "  [!] Acces refuse.\n"; continue; }

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
            req["invite_code"] = g_inviteCode;
            req["token"] = g_token;

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
            req["invite_code"] = g_inviteCode;
            req["token"] = g_token;
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
                req["invite_code"] = g_inviteCode;
                req["token"] = g_token;
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
            req["invite_code"] = g_inviteCode;
            req["token"] = g_token;
            req["index"] = index;

            sendJson(sock, req.dump());

            std::string resp;
            recvJson(sock, resp);
            printServerResponse(resp);
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