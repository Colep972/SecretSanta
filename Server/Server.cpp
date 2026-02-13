#include <iostream>
#include <unordered_map>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "../external/json.hpp"
#include "NetworkUtils.h"
#include "Crew.h"
#include "Users.h"
#include "Save.h"
#include "Draw.h"
#include "Mailer.h"


using json = nlohmann::json;

#pragma comment(lib, "ws2_32.lib")

// =========================
// Génération code invitation
// =========================
std::string generateInviteCode()
{
    const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string code = "SANTA-";

    for (int i = 0; i < 6; i++)
        code += chars[rand() % (sizeof(chars) - 1)];

    return code;
}

int main()
{
    WSADATA wsData;
    WSAStartup(MAKEWORD(2, 2), &wsData);

    SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in hint{};
    hint.sin_family = AF_INET;
    hint.sin_port = htons(54000);
    hint.sin_addr.S_un.S_addr = INADDR_ANY;

    bind(listening, (sockaddr*)&hint, sizeof(hint));
    listen(listening, SOMAXCONN);

    std::cout << "Serveur pret.\n";

    // =========================
    // Table invitations en mémoire
    // invite_code -> fichier crew
    // =========================
    std::unordered_map<std::string, std::string> invites;

    while (true)
    {
        SOCKET client = accept(listening, nullptr, nullptr);
        std::cout << "Client connecte\n";

        std::string msg;
        while (recvJson(client, msg))
        {
            std::cout << "Recu: " << msg << "\n";

            json request;
            try
            {
                request = json::parse(msg);
            }
            catch (...)
            {
                sendJson(client,
                    R"({"status":"ERROR","message":"JSON invalide"})");
                continue;
            }

            if (!request.contains("action"))
            {
                sendJson(client,
                    R"({"status":"ERROR","message":"Action manquante"})");
                continue;
            }

            std::string action = request["action"];

            // =========================
            // PING
            // =========================
            if (action == "PING")
            {
                sendJson(client,
                    R"({"status":"OK","reply":"PONG"})");
            }

            // =========================
            // CREATE_CREW
            // =========================
            else if (action == "CREATE_CREW")
            {
                if (!request.contains("crew_name"))
                {
                    sendJson(client,
                        R"({"status":"ERROR","message":"crew_name manquant"})");
                    continue;
                }

                std::string crewName = request["crew_name"];
                std::string inviteCode = generateInviteCode();
                std::string crewFile =
                    "ServerData/Crews/" + inviteCode + ".json";

                Crew crew(crewName);
                Save::saveCrew(crew, crewFile);

                invites[inviteCode] = crewFile;

                json response;
                response["status"] = "OK";
                response["invite_code"] = inviteCode;

                sendJson(client, response.dump());
            }

            // =========================
            // JOIN_CREW
            // =========================
            else if (action == "JOIN_CREW")
            {
                if (!request.contains("invite_code") ||
                    !request.contains("user"))
                {
                    sendJson(client,
                        R"({"status":"ERROR","message":"Parametres manquants"})");
                    continue;
                }

                std::string inviteCode = request["invite_code"];

                if (invites.find(inviteCode) == invites.end())
                {
                    sendJson(client,
                        R"({"status":"ERROR","message":"Code invalide"})");
                    continue;
                }

                auto userJson = request["user"];

                Users user(
                    userJson["name"].get<std::string>(),
                    userJson["email"].get<std::string>()
                );

                Crew crew;
                Save::loadCrew(crew, invites[inviteCode]);
                crew.addUser(user);
                Save::saveCrew(crew, invites[inviteCode]);

                sendJson(client,
                    R"({"status":"OK"})");
            }
            else if (action == "RUN_DRAW")
            {
                if (!request.contains("invite_code"))
                {
                    sendJson(client,
                        R"({"status":"ERROR","message":"invite_code manquant"})");
                    continue;
                }

                std::string inviteCode = request["invite_code"];

                if (invites.find(inviteCode) == invites.end())
                {
                    sendJson(client,
                        R"({"status":"ERROR","message":"Code invalide"})");
                    continue;
                }

                Crew crew;
                Save::loadCrew(crew, invites[inviteCode]);
                std::cout << "[DRAW] Participants:\n";
                for (const auto& u : crew.getUsers())
                {
                    std::cout << " - " << u.getName() << "\n";
                }


                Draw draw;
                if (!draw.run(crew))
                {
                    sendJson(client,
                        R"({"status":"ERROR","message":"Erreur tirage"})");
                    continue;
                }

                Save::saveDrawResult(
                    invites[inviteCode],
                    draw.getResults());

                sendJson(client,
                    R"({"status":"OK","message":"Tirage effectue"})");
            }
            else if (action == "SEND_EMAILS")
            {
                if (!request.contains("invite_code"))
                {
                    sendJson(client,
                        R"({"status":"ERROR","message":"invite_code manquant"})");
                    continue;
                }

                std::string inviteCode = request["invite_code"];

                if (invites.find(inviteCode) == invites.end())
                {
                    sendJson(client,
                        R"({"status":"ERROR","message":"Code invalide"})");
                    continue;
                }

                std::string crewFile = invites[inviteCode];

                Crew crew;
                Save::loadCrew(crew, crewFile);

                std::map<std::string, std::string> results;
                if (!Save::loadDrawResult(crewFile, results))
                {
                    sendJson(client,
                        R"({"status":"ERROR","message":"Aucun tirage trouve"})");
                    continue;
                }

                Mailer mailer;

                for (const auto& user : crew.getUsers())
                {
                    const std::string& giver = user.getName();

                    if (results.find(giver) == results.end())
                        continue;

                    const std::string& receiver = results[giver];

                    mailer.sendOne(
                        user.getEmail(),
                        giver,
                        receiver
                    );
                }

                sendJson(client,
                    R"({"status":"OK","message":"Emails envoyes"})");
                    }

            // =========================
            // ACTION INCONNUE
            // =========================
            else
            {
                sendJson(client,
                    R"({"status":"ERROR","message":"Action inconnue"})");
            }
        }

        closesocket(client);
        std::cout << "Client deconnecte\n";
    }

    WSACleanup();
    return 0;
}

