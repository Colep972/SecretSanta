#include <iostream>
#include <unordered_map>
#include <string>
#include <fstream>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "NetworkUtils.h"
#include "Crew.h"
#include "Users.h"
#include "Save.h"
#include "Draw.h"

// =========================
// Génération code invitation
// =========================

static const char* INVITES_FILE = "ServerData/invites.json";

static void saveInvites(const std::unordered_map<std::string, std::string>& invites)
{
    json j = json::object();
    for (const auto& kv : invites)
        j[kv.first] = kv.second;

    std::ofstream out(INVITES_FILE);
    out << j.dump(4);
}

static void loadInvites(std::unordered_map<std::string, std::string>& invites)
{
    invites.clear();

    std::ifstream in(INVITES_FILE);
    if (!in.is_open())
        return;

    try
    {
        json j;
        in >> j;

        for (auto it = j.begin(); it != j.end(); ++it)
            invites[it.key()] = it.value().get<std::string>();
    }
    catch (...)
    {
        invites.clear(); // fichier corrompu => on repart vide
    }
}

const std::string ADMIN_TOKEN = "SANTA-ADMIN-I-KNOW-COLEP-972-/-MATHIEU";

static std::string generateInviteCode()
{
    static const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string code = "SANTA-";
    for (int i = 0; i < 6; i++)
        code += chars[rand() % (sizeof(chars) - 1)];
    return code;
}

int main()
{
    // Petite seed pour generateInviteCode()
    srand((unsigned)time(nullptr));

    int listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening < 0)
    {
        std::cerr << "socket() failed\n";
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(54000);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listening, (sockaddr*)&addr, sizeof(addr)) < 0)
    {
        std::cerr << "bind() failed (port deja pris ?)\n";
        close(listening);
        return 1;
    }

    if (listen(listening, SOMAXCONN) < 0)
    {
        std::cerr << "listen() failed\n";
        close(listening);
        return 1;
    }

    std::cout << "Serveur prêt (Raspberry Pi) sur le port 54000\n";

    // invite_code -> chemin du crew
    std::unordered_map<std::string, std::string> invites;
    loadInvites(invites);
    std::cout << "Invites charges: " << invites.size() << "\n";

    while (true)
    {
        int client = accept(listening, nullptr, nullptr);
        if (client < 0)
            continue;

        std::cout << "Client connecté\n";

        std::string msg;
        while (recvJson(client, msg))
        {
            std::cout << "Reçu: " << msg << "\n";

            json request;
            try
            {
                request = json::parse(msg);
            }
            catch (...)
            {
                sendJson(client, R"({"status":"ERROR","message":"JSON invalide"})");
                continue;
            }

            if (!request.contains("action"))
            {
                sendJson(client, R"({"status":"ERROR","message":"action manquante"})");
                continue;
            }

            std::string action = request["action"].get<std::string>();

            if (action == "PING")
            {
                sendJson(client, R"({"status":"OK","reply":"PONG"})");
            }
            else if (action == "CREATE_CREW")
            {
                if (!request.contains("crew_name"))
                {
                    sendJson(client, R"({"status":"ERROR","message":"crew_name manquant"})");
                    continue;
                }

                std::string crewName = request["crew_name"].get<std::string>();
                std::string inviteCode = generateInviteCode();
                std::string crewFile = "ServerData/Crews/" + inviteCode + ".json";

                Crew crew(crewName);

                if (request.contains("user"))
                {
                    json u = request["user"];
                    if (u.contains("name") && u.contains("email"))
                    {
                        Users creator(u["name"].get<std::string>(), u["email"].get<std::string>());
                        crew.addUser(creator);
                    }
                }

                Save::saveCrew(crew, crewFile);

                invites[inviteCode] = crewFile;
                saveInvites(invites); // si tu as la persistance des invites

                json resp;
                resp["status"] = "OK";
                resp["invite_code"] = inviteCode;

                // optionnel : pour feedback UX
                resp["participants_count"] = crew.getUsers().size();

                sendJson(client, resp.dump());
            }
            else if (action == "JOIN_CREW")
            {
                if (!request.contains("invite_code") || !request.contains("user"))
                {
                    sendJson(client, R"({"status":"ERROR","message":"Parametres manquants"})");
                    continue;
                }

                std::string inviteCode = request["invite_code"].get<std::string>();
                auto it = invites.find(inviteCode);
                if (it == invites.end())
                {
                    sendJson(client, R"({"status":"ERROR","message":"Code invalide"})");
                    continue;
                }

                json u = request["user"];
                if (!u.contains("name") || !u.contains("email"))
                {
                    sendJson(client, R"({"status":"ERROR","message":"user incomplet"})");
                    continue;
                }

                Users user(u["name"].get<std::string>(), u["email"].get<std::string>());

                Crew crew;
                Save::loadCrew(crew, it->second);
                crew.addUser(user);
                Save::saveCrew(crew, it->second);

                json resp;
                resp["status"] = "OK";
                resp["participants_count"] = crew.getUsers().size();
                sendJson(client, resp.dump());
            }
            else if (action == "RUN_DRAW")
            {
                if (!request.contains("admin_token") ||
                    request["admin_token"].get<std::string>() != ADMIN_TOKEN)
                {
                    sendJson(client, R"({"status":"ERROR","message":"Non autorise"})");
                    continue;
                }

                if (!request.contains("invite_code"))
                {
                    sendJson(client, R"({"status":"ERROR","message":"invite_code manquant"})");
                    continue;
                }

                std::string inviteCode = request["invite_code"].get<std::string>();
                auto it = invites.find(inviteCode);
                if (it == invites.end())
                {
                    sendJson(client, R"({"status":"ERROR","message":"Code invalide"})");
                    continue;
                }

                Crew crew;
                Save::loadCrew(crew, it->second);

                if (crew.getUsers().size() < 3)
                {
                    sendJson(client, R"({"status":"ERROR","message":"Minimum 3 participants requis"})");
                    continue;
                }

                Draw draw;
                if (!draw.run(crew))
                {
                    sendJson(client, R"({"status":"ERROR","message":"Erreur tirage"})");
                    continue;
                }

                if (!Save::saveDrawResult(it->second, draw.getResults()))
                {
                    sendJson(client, R"({"status":"ERROR","message":"Erreur sauvegarde tirage"})");
                    continue;
                }

                sendJson(client, R"({"status":"OK","message":"Tirage effectue"})");
            }
            else if (action == "SEND_EMAILS")
            {
                if (!request.contains("invite_code"))
                {
                    sendJson(client, R"({"status":"ERROR","message":"invite_code manquant"})");
                    continue;
                }

                std::string inviteCode = request["invite_code"].get<std::string>();
                auto it = invites.find(inviteCode);
                if (it == invites.end())
                {
                    sendJson(client, R"({"status":"ERROR","message":"Code invalide"})");
                    continue;
                }

                Crew crew;
                Save::loadCrew(crew, it->second);

                std::unordered_map<std::string, std::string> results;
                if (!Save::loadDrawResult(it->second, results))
                {
                    sendJson(client, R"({"status":"ERROR","message":"Tirage introuvable"})");
                    continue;
                }

                Mailer mailer;
                if (!mailer.send(crew, results))
                {
                    sendJson(client, R"({"status":"ERROR","message":"Erreur envoi emails"})");
                    continue;
                }

                sendJson(client, R"({"status":"OK","message":"Emails envoyes"})");
                }
            else
            {
                sendJson(client, R"({"status":"ERROR","message":"Action inconnue"})");
            }

        }
        close(client);
        std::cout << "Client déconnecté\n";
    }

    close(listening);
    return 0;
}