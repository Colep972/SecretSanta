#include "SecretSanta.h"
#include <stdlib.h>
#include <time.h>
#include <string>
#include <mailio/message.hpp>
#include <mailio/smtp.hpp>

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

void SecretSanta::SendEmails()
{
    try
    {
        std::string sender = "joakim.alves972@gmail.com";
        std::string pass = "hizuaddrdddkvkwp";
        std::cout << "len pass = " << pass.size() << std::endl;

        mailio::smtps smtp_client("smtp.gmail.com", 587);
        smtp_client.authenticate(sender, pass, mailio::smtps::auth_method_t::START_TLS);

        for (int i = 0; i < m_nbParticipants; i++) 
        { 
            std::string giver = participants[i];
            std::string receiver = stock[i];
            camille[giver] = receiver;
            mailio::message msg; 
            msg.from(mailio::mail_address("Secret Santa", sender)); 
            msg.add_recipient(mailio::mail_address("Secret Santa", santaMap[giver]));
            msg.subject("Ton Secret Santa"); 
            msg.content("Bonjour " + giver + " tu offres a : " + receiver);
            smtp_client.submit(msg); 
        }
        Camille();
    }
    catch (const mailio::smtp_error& e)
    {
        std::cerr << "SMTP ERROR: " << e.what() << std::endl;

    }
    catch (const std::exception& e)
    {
        std::cerr << "OTHER ERROR: " << e.what() << std::endl;
    }
}



void SecretSanta::Camille()
{
    std::string pass = "hizuaddrdddkvkwp";
    mailio::smtps smtp_client("smtp.gmail.com", 587);
    smtp_client.authenticate("joakim.alves972@gmail.com", pass, mailio::smtps::auth_method_t::START_TLS);
    for (int i = 0; i < camille.size(); i++)
    {
        std::string giver = participants[i];
        std::string receiver = stock[i];
        mailio::message msg; 
        msg.from(mailio::mail_address("Secret Santa", "joakim.alves972@gmail.com"));
        msg.add_recipient(mailio::mail_address("Secret Santa", santaMap[giver]));
        std::cout << santaMap[giver] << std::endl;
        msg.subject("Ton Secret Santa"); 
        msg.content("Bonjour " + giver + " offres a : " + receiver);
        smtp_client.submit(msg);
    }
}

int SecretSanta::getnbParticipants()
{
	return m_nbParticipants;
}