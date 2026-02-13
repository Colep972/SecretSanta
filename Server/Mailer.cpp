#include "Mailer.h"
#include <mailio/message.hpp>
#include <mailio/smtp.hpp>

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

                mailio::message msg;
                msg.from(mailio::mail_address("Secret Santa", sender));
                msg.add_recipient(
                    mailio::mail_address(giver, user.getEmail())
                );

                msg.subject("Ton Secret Santa");
                msg.content(
                    "Bonjour " + giver +
                    ",Tu offres un cadeau à : " + receiver +
                    "  Joyeux Secret Santa !"
                );

                smtp_client.submit(msg);
                std::cout << "Mails envoyés avec succès ! " << std::endl;
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
        "Bonjour " + giver +
        "Tu es le Secret Santa de : " + receiver +
        "Joyeuses fetes !"
        );

        smtp_client.submit(msg);
        std::cout << "Mails envoyés avec succès ! " << std::endl;
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
