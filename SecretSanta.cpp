#include "SecretSanta.h"
#include <stdlib.h>
#include <time.h>
#include <string>
#include <mailio/message.h>
#include <mailio/smtp.h>

SecretSanta::SecretSanta() :m_nbParticipants(0)
{

}

SecretSanta::~SecretSanta()
{

}

bool SecretSanta::init()
{
	int n = 0;
	int nbParticipants = 0;
	std::string name;
	std::string email;
	std::cout << "Bienvenue sur SecretSanta.cpp. Veuillez saisir le nombre de participants" << std::endl;
	std::cin >> nbParticipants;
	m_nbParticipants = nbParticipants;
	while (nbParticipants != 0)
	{
		std::cout << "Saisissez le participant n° " << n << " : " << std::endl;
		std::cin >> name;
		participants.push_back(name);
		n++;
		nbParticipants--;
        std::cout << "L'email maintenant" << std::endl;
        std::cin >> email;
        santaMap[name] = email;
	}
	std::cout << "C'est tout bon !" << std::endl;
	return true;
}

void SecretSanta::Pull()
{
    stock.clear();

    // On copie la liste des participants dans "stock"
    stock = participants;

    // Mélange aléatoire
    srand(time(NULL));
    for (int i = stock.size() - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        std::swap(stock[i], stock[j]);
    }

    // Vérifie qu'aucun participant ne tombe sur lui-même
    bool valid = false;
    while (!valid)
    {
        valid = true;

        for (int i = 0; i < m_nbParticipants; i++)
        {
            if (stock[i] == participants[i])  // même personne
            {
                valid = false;
                break;
            }
        }

        if (!valid) 
        {
            for (int i = stock.size() - 1; i > 0; i--) 
            {
                int j = rand() % (i + 1);
                std::swap(stock[i], stock[j]);
            }
        }
    }
}


void SecretSanta::Show()
{
    // Pour faire un cercle, le dernier offre au premier
    for (int i = 0; i < m_nbParticipants; i++)
    {
        int j = (i + 1) % m_nbParticipants;
        std::cout << stock[i] << " qui a pour email " << santaMap[stock[i]] << " offre un cadeau à " << stock[j] << " qui a pour email " << santaMap[stock[j]] << std::endl;
    }
}


int SecretSanta::getnbParticipants()
{
	return m_nbParticipants;
}