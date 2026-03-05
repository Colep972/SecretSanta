#include "Mailer.h"
#include <mailio/message.hpp>
#include <mailio/smtp.hpp>
#include <iostream>

bool Mailer::send(const Crew& crew, const std::map<std::string, std::string>& drawResults)
{
    try
    {
        std::string sender = "joakim.alves972@gmail.com";
        std::string pass = "hizuaddrdddkvkwp";

        mailio::smtps smtp_client("smtp.gmail.com", 587);
        smtp_client.authenticate(sender, pass, mailio::smtps::auth_method_t::START_TLS);

        const auto& users = crew.getUsers();

        for (const auto& user : users)
        {
            const std::string& giver = user.getName();

            auto it = drawResults.find(giver);
            if (it == drawResults.end())
                continue;

            const std::string& receiver = it->second;

            // Find receiver's wish list
            const Users* receiverUser = nullptr;
            for (const auto& u : users)
                if (u.getName() == receiver) { receiverUser = &u; break; }

            std::string wishText;
            if (receiverUser && !receiverUser->getWishes().empty())
            {
                wishText = "\n\nVoici la liste de souhaits de " + receiver + " :\n";
                int idx = 1;
                for (const auto& w : receiverUser->getWishes())
                    wishText += "  " + std::to_string(idx++) + ". " + w + "\n";
            }
            else
            {
                wishText = "\n\n" + receiver + " n'a pas encore renseigne de liste de souhaits.";
            }

            mailio::message msg;
            msg.from(mailio::mail_address("Secret Santa", sender));
            msg.add_recipient(mailio::mail_address(giver, user.getEmail()));
            msg.subject("Ton Secret Santa");
            msg.content(
                "Bonjour " + giver + ",\n\n"
                "Tu offres un cadeau a : " + receiver +
                wishText +
                "\n\nJoyeux Secret Santa !"
            );

            smtp_client.submit(msg);
            std::cout << "Mail envoye a " << giver << "\n";
        }

        return true;
    }
    catch (const mailio::smtp_error& e)
    {
        std::cerr << "SMTP ERROR: " << e.what() << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "OTHER ERROR: " << e.what() << std::endl;
    }
    return false;
}

bool Mailer::sendOne(const std::string& to, const std::string& giver, const std::string& receiver)
{
    try
    {
        std::string sender = "joakim.alves972@gmail.com";
        std::string pass = "hizuaddrdddkvkwp";

        mailio::smtps smtp_client("smtp.gmail.com", 587);
        smtp_client.authenticate(sender, pass, mailio::smtps::auth_method_t::START_TLS);

        mailio::message msg;
        msg.from(mailio::mail_address("Secret Santa", sender));
        msg.add_recipient(mailio::mail_address("", to));
        msg.subject("Ton Secret Santa");
        msg.content(
            "Bonjour " + giver + ",\n\n"
            "Tu es le Secret Santa de : " + receiver + "\n\n"
            "Joyeuses fetes !"
        );

        smtp_client.submit(msg);
        std::cout << "Mail envoye a " << to << "\n";
        return true;
    }
    catch (const mailio::smtp_error& e)
    {
        std::cerr << "SMTP ERROR: " << e.what() << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "OTHER ERROR: " << e.what() << std::endl;
    }
    return false;
}