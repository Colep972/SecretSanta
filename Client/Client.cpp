#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "NetworkUtils.h"
#include "../external/json.hpp"

#pragma comment(lib, "ws2_32.lib")

using json = nlohmann::json;

int main()
{
    WSADATA wsData;
    WSAStartup(MAKEWORD(2, 2), &wsData);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(54000);
    InetPton(AF_INET, L"127.0.0.1", &server.sin_addr);

    if (connect(sock, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
    {
        std::cerr << "Connexion impossible\n";
        return -1;
    }

    std::cout << "Connecte au serveur\n";

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
            std::string crewName;
            std::cout << "Nom du crew : ";
            std::cin >> crewName;

            json req;
            req["action"] = "CREATE_CREW";
            req["crew_name"] = crewName;

            sendJson(sock, req.dump());

            std::string resp;
            recvJson(sock, resp);

            json res = json::parse(resp);
            std::cout << "Invite code : "
                << res["invite_code"] << "\n";
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

            json res = json::parse(resp);
            std::cout << res.dump(4) << "\n";
        }
        else if (choice == 3)
        {
            std::string code;
            std::cout << "Code du crew : ";
            std::cin >> code;

            json req;
            req["action"] = "RUN_DRAW";
            req["invite_code"] = code;

            sendJson(sock, req.dump());

            std::string resp;
            recvJson(sock, resp);

            json res = json::parse(resp);
            std::cout << res["message"] << std::endl;
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

            json res = json::parse(resp);
            std::cout << res["message"] << std::endl;

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
