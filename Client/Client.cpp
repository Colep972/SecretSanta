#include <limits>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "NetworkUtils.h"
#include "../external/json.hpp"

#pragma comment(lib, "ws2_32.lib")

using json = nlohmann::json;

void printServerResponse(const std::string& resp)
{
    json res = json::parse(resp);

    if (!res.contains("status"))
    {
        std::cout << "[ERREUR] Reponse invalide\n";
        return;
    }

    std::string status = res["status"];

    if (status == "OK")
        std::cout << "[OK] ";
    else
        std::cout << "[ERREUR] ";

    if (res.contains("message"))
        std::cout << res["message"];

    std::cout << std::endl;
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

    std::cout << "Connectxion au serveur etablie !\n";

    bool running = true;

    while (running)
    {
        std::cout << std::endl << "1. Creer un crew " << std::endl;
        std::cout << "2. Rejoindre un crew " << std::endl;
        std::cout << "3. Lancer le tirage " << std::endl;
        std::cout << "4. Envoyer les emails " << std::endl;
        std::cout << "5. Quitter " << std::endl;
        std::cout << "Choix : ";

        int choice;
        std::cin >> choice;

        if (choice == 1)
        {
            std::string crewName, name, email;

            std::cout << "Nom du crew : ";
            std::getline(std::cin >> std::ws, crewName);

            std::cout << "Ton nom : ";
            std::getline(std::cin >> std::ws, name);

            std::cout << "Ton email : ";
            std::getline(std::cin >> std::ws, email);

            json req;
            req["action"] = "CREATE_CREW";
            req["crew_name"] = crewName;
            req["user"]["name"] = name;
            req["user"]["email"] = email;

            sendJson(sock, req.dump());

            std::string resp;
            recvJson(sock, resp);
            printServerResponse(resp);
        }
        else if (choice == 2)
        {
            std::string code, name, email;

            std::cout << "Code : ";
            std::cin >> code;
            std::cout << "Nom : ";
            std::cin >> name;
            std::cout << "Email : ";
            std::cin >> email;

            json req;
            req["action"] = "JOIN_CREW";
            req["invite_code"] = code;
            req["user"]["name"] = name;
            req["user"]["email"] = email;

            sendJson(sock, req.dump());
            std::string resp;
            recvJson(sock, resp);
            printServerResponse(resp);
        }
        else if (choice == 3)
        {
            std::string code;
            std::cout << "Code du crew : ";
            std::cin >> code;

            json req;
            req["action"] = "RUN_DRAW";
            req["invite_code"] = code;
            req["admin_token"] = "SANTA-ADMIN-I-KNOW-COLEP-972-/-MATHIEU";

            sendJson(sock, req.dump());

            std::string resp;
            recvJson(sock, resp);
            printServerResponse(resp);
        }
        else if (choice == 4)
        {
            std::string code;
            std::cout << "Code du crew : ";
            std::cin >> code;

            json req;
            req["action"] = "SEND_EMAILS";
            req["invite_code"] = code;

            sendJson(sock, req.dump());

            std::string resp;
            recvJson(sock, resp);
            printServerResponse(resp);
        }
        else if (choice == 5)
        {
            running = false;
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
