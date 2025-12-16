#include <iostream>
#include "Profile.h"
#include "Save.h"
#include "Crew.h"
#include "Users.h"
#include "Draw.h"
#include "Mailer.h"

int main()
{
    std::cout << "=== Secret Santa ===\n";

    std::string username;
    std::cout << "Nom du profil : ";
    std::cin >> username;

    std::string profileFile = "Profiles/" + username + ".json";
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
        std::cin >> choice;

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
        std::cin >> menu;

        if (menu == 1)
        {
            std::string crewName;
            std::cout << "Nom du crew : ";
            std::cin >> crewName;

            Crew crew(crewName);

            int nbParticipants;
            std::cout << "Nombre de participants : ";
            std::cin >> nbParticipants;

            for (int i = 0; i < nbParticipants; i++)
            {
                std::string name, email;
                std::cout << "Nom participant " << i + 1 << " : ";
                std::cin >> name;
                std::cout << "Email : ";
                std::cin >> email;

                crew.addUser(Users(name, email));
            }

            std::string crewFile = "Crews/" + crewName + ".json";
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

