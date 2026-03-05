#include <iostream>
#include <unordered_map>
#include <string>
#include <fstream>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "NetworkUtils.h"
#include "Crew.h"
#include "Users.h"
#include "Save.h"
#include "Draw.h"
#include "Mailer.h"

// =========================
// G�n�ration code invitation
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

static std::string generateToken()
{
    static const char chars[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    std::string token;
    for (int i = 0; i < 16; i++)
        token += chars[rand() % (sizeof(chars) - 1)];
    return token;
}

int main()
{
    // Petite seed pour generateInviteCode()
    srand((unsigned)time(nullptr));

    int listening = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listening, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
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

    std::cout << "Serveur pr�t (Raspberry Pi) sur le port 54000\n";

    // invite_code -> chemin du crew
    std::unordered_map<std::string, std::string> invites;
    loadInvites(invites);
    std::cout << "Invites charges: " << invites.size() << "\n";

    while (true)
    {
        int client = accept(listening, nullptr, nullptr);
        if (client < 0)
            continue;

        std::cout << "Client connect�\n";

        std::string msg;
        while (recvJson(client, msg))
        {
            std::cout << "Re�u: " << msg << "\n";

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

                json resp;
                resp["status"] = "OK";
                resp["invite_code"] = inviteCode;

                if (request.contains("user"))
                {
                    json u = request["user"];
                    if (u.contains("name") && u.contains("email"))
                    {
                        Users creator(u["name"].get<std::string>(), u["email"].get<std::string>());
                        creator.setToken(generateToken());
                        crew.addUser(creator);

                        // First user is the owner
                        crew.setOwnerToken(creator.getToken());
                        resp["is_owner"] = true;
                        resp["token"] = creator.getToken();
                    }
                }

                Save::saveCrew(crew, crewFile);

                invites[inviteCode] = crewFile;
                saveInvites(invites);

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
                user.setToken(generateToken());

                Crew crew;
                Save::loadCrew(crew, it->second);
                crew.addUser(user);
                Save::saveCrew(crew, it->second);

                json resp;
                resp["status"] = "OK";
                resp["token"] = user.getToken();
                resp["is_owner"] = false;
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

                std::map<std::string, std::string> results;
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
            else if (action == "LOGIN_CREW")
            {
                // Required: invite_code, name
                // Finds the participant by name and returns their token
                if (!request.contains("invite_code") || !request.contains("name"))
                {
                    sendJson(client, R"({"status":"ERROR","message":"invite_code et name requis"})");
                    continue;
                }

                std::string inviteCode = request["invite_code"].get<std::string>();
                auto it = invites.find(inviteCode);
                if (it == invites.end())
                {
                    sendJson(client, R"({"status":"ERROR","message":"Code invalide"})");
                    continue;
                }

                std::string name = request["name"].get<std::string>();

                Crew crew;
                Save::loadCrew(crew, it->second);

                const Users* user = crew.findUserByName(name);
                if (!user)
                {
                    sendJson(client, R"({"status":"ERROR","message":"Nom introuvable dans ce crew"})");
                    continue;
                }

                json resp;
                resp["status"] = "OK";
                resp["token"] = user->getToken();
                resp["is_owner"] = (user->getToken() == crew.getOwnerToken());
                resp["participants_count"] = crew.getUsers().size();
                sendJson(client, resp.dump());
            }
            else if (action == "ADD_WISH")
            {
                // Required: invite_code, token, wish
                if (!request.contains("invite_code") || !request.contains("token") || !request.contains("wish"))
                {
                    sendJson(client, R"({"status":"ERROR","message":"invite_code, token et wish requis"})");
                    continue;
                }

                std::string inviteCode = request["invite_code"].get<std::string>();
                auto it = invites.find(inviteCode);
                if (it == invites.end())
                {
                    sendJson(client, R"({"status":"ERROR","message":"Code invalide"})");
                    continue;
                }

                std::string token = request["token"].get<std::string>();
                std::string wish = request["wish"].get<std::string>();

                Crew crew;
                Save::loadCrew(crew, it->second);

                Users* user = crew.findUserByToken(token);
                if (!user)
                {
                    sendJson(client, R"({"status":"ERROR","message":"Token invalide"})");
                    continue;
                }

                user->addWish(wish);
                Save::saveCrew(crew, it->second);

                json resp;
                resp["status"] = "OK";
                resp["wishes"] = user->getWishes();
                sendJson(client, resp.dump());
            }
            else if (action == "REMOVE_WISH")
            {
                // Required: invite_code, token, index (0-based)
                if (!request.contains("invite_code") || !request.contains("token") || !request.contains("index"))
                {
                    sendJson(client, R"({"status":"ERROR","message":"invite_code, token et index requis"})");
                    continue;
                }

                std::string inviteCode = request["invite_code"].get<std::string>();
                auto it = invites.find(inviteCode);
                if (it == invites.end())
                {
                    sendJson(client, R"({"status":"ERROR","message":"Code invalide"})");
                    continue;
                }

                std::string token = request["token"].get<std::string>();
                int index = request["index"].get<int>();

                Crew crew;
                Save::loadCrew(crew, it->second);

                Users* user = crew.findUserByToken(token);
                if (!user)
                {
                    sendJson(client, R"({"status":"ERROR","message":"Token invalide"})");
                    continue;
                }

                if (!user->removeWish(index))
                {
                    sendJson(client, R"({"status":"ERROR","message":"Index invalide"})");
                    continue;
                }

                Save::saveCrew(crew, it->second);

                json resp;
                resp["status"] = "OK";
                resp["wishes"] = user->getWishes();
                sendJson(client, resp.dump());
            }
            else if (action == "GET_WISHES")
            {
                // Required: invite_code, token
                // Returns the caller's own wish list
                if (!request.contains("invite_code") || !request.contains("token"))
                {
                    sendJson(client, R"({"status":"ERROR","message":"invite_code et token requis"})");
                    continue;
                }

                std::string inviteCode = request["invite_code"].get<std::string>();
                auto it = invites.find(inviteCode);
                if (it == invites.end())
                {
                    sendJson(client, R"({"status":"ERROR","message":"Code invalide"})");
                    continue;
                }

                std::string token = request["token"].get<std::string>();

                Crew crew;
                Save::loadCrew(crew, it->second);

                const Users* user = crew.findUserByToken(token);
                if (!user)
                {
                    sendJson(client, R"({"status":"ERROR","message":"Token invalide"})");
                    continue;
                }

                json resp;
                resp["status"] = "OK";
                resp["name"] = user->getName();
                resp["wishes"] = user->getWishes();
                sendJson(client, resp.dump());
            }
            else if (action == "GET_CREW_STATUS")
            {
                // Required: invite_code
                // Returns participant count and whether draw has been done
                if (!request.contains("invite_code"))
                {
                    sendJson(client, R"({"status":"ERROR","message":"invite_code requis"})");
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

                std::map<std::string, std::string> drawResults;
                bool drawDone = Save::loadDrawResult(it->second, drawResults);

                json resp;
                resp["status"] = "OK";
                resp["participants_count"] = crew.getUsers().size();
                resp["draw_done"] = drawDone;
                sendJson(client, resp.dump());
            }
            else
            {
                sendJson(client, R"({"status":"ERROR","message":"Action inconnue"})");
            }

        }
        close(client);
        std::cout << "Client d�connect�\n";
    }

    close(listening);
    return 0;
}