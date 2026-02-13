#include <iostream>
#include "Profile.h"
#include "Save.h"
#include "Crew.h"
#include "Users.h"
#include "Draw.h"
#include "Mailer.h"

std::string sanitizeFilename(std::string s)
{
    for (char& c : s)
    {
        if (c == ' ')
            c = '_';
    }
    return s;
}

int main()
{
    std::cout << "=== Secret Santa ===\n";

    std::string username;
    std::cout << "Nom du profil : ";
    std::getline(std::cin >> std::ws,username);

    std::string profileFile = "Profiles/" + sanitizeFilename(username) + ".json";
    Profile profile;

    // =========================
    // Gestion du profil
    // =========================
    if (Save::profileExists(profileFile))
    {
        int choice;
        std::cout << "\nProfil existant.\n";
        std::cout << "1. Charger le profil\n";
        std::cout << "2. Creer un nouveau profil\n";
        std::cout << "Choix : ";
        while (!(std::cin >> choice) || choice <= 0)
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Nombre invalide, recommence : ";
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (choice == 1)
        {
            if (!Save::loadProfile(profile, profileFile))
            {
                std::cerr << "Erreur chargement profil\n";
                return -1;
            }
            std::cout << "Profil charge.\n";
        }
        else
        {
            profile = Profile(username);
            Save::saveProfile(profile, profileFile);
            std::cout << "Nouveau profil cree.\n";
        }
    }
    else
    {
        profile = Profile(username);
        Save::saveProfile(profile, profileFile);
        std::cout << "Nouveau profil cree.\n";
    }

    // =========================
    // Menu principal (version simple)
    // =========================
    int menu = 0;
    while (menu != 3)
    {
        std::cout << std::endl << " --- Menu --- " << std::endl;
        std::cout << "1. Creer un crew " << std::endl;
        std::cout << "2. Charger un crew " << std::endl;
        std::cout << "3. Quitter " << std::endl;
        std::cout << "Choix : ";
        while (!(std::cin >> menu) || menu <= 0)
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Nombre invalide, recommence : ";
        }

        if (menu == 1)
        {
            std::string crewName;
            std::cout << "Nom du crew : ";
            std::getline(std::cin >> std::ws, crewName);

            Crew crew(crewName);

            int nbParticipants;
            std::cout << "Nombre de participants : ";
            while (!(std::cin >> nbParticipants) || nbParticipants <= 0)
            {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "Nombre invalide, recommence : ";
            }

            for (int i = 0; i < nbParticipants; i++)
            {
                std::string name, email;
                std::cout << "Nom participant " << i + 1 << " : ";
                std::getline(std::cin >> std::ws, name);
                std::cout << "Email : ";
                std::cin >> email;

                crew.addUser(Users(name, email));
            }

            std::string crewFile =
                "Crews/" + sanitizeFilename(crewName) + ".json";
            Save::saveCrew(crew, crewFile);
            profile.addCrewFile(crewFile);
            Save::saveProfile(profile, profileFile);

            Draw draw;
            draw.run(crew);

            Mailer mailer;
            mailer.send(crew, draw.getResults());
        }

        else if (menu == 2)
        {
            const auto& crews = profile.getCrewFiles();
            if (crews.empty())
            {
                std::cout << "Aucun crew.\n";
                continue;
            }

            std::cout << "Crews disponibles :\n";
            for (size_t i = 0; i < crews.size(); i++)
                std::cout << i + 1 << ". " << crews[i] << "\n";

            int index;
            std::cout << "Choix : ";
            std::cin >> index;

            if (index < 1 || index > crews.size())
                continue;

            Crew crew;
            Save::loadCrew(crew, crews[index - 1]);

            Draw draw;
            draw.run(crew);

            Mailer mailer;
            mailer.send(crew, draw.getResults());
        }
    }

    return 0;
}

