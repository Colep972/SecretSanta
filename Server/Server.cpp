#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main()
{
    int listening = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(54000);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(listening, (sockaddr*)&addr, sizeof(addr));
    listen(listening, SOMAXCONN);

    std::cout << "Serveur prêt (Raspberry Pi)\n";

    while (true)
    {
        int client = accept(listening, nullptr, nullptr);
        std::cout << "Client connecté\n";

        char buffer[4096];
        int bytes = recv(client, buffer, 4096, 0);
        if (bytes > 0)
        {
            send(client, buffer, bytes, 0);
        }

        close(client);
    }

    close(listening);
    return 0;
}