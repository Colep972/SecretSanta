#define NOMINMAX
#include <iostream>
#include <limits>
#include <string>
#include "NetworkUtils.h"
#include "../external/json.hpp"

using json = nlohmann::json;

static const std::string ADMIN_TOKEN = "GENIE-ADMIN-I-KNOW-COLEP-972-/-MATHIEU";

// Session state
static std::string m_profileToken;
static std::string m_name;
static std::string m_email;
static std::string m_inviteCode;
static std::string m_crewToken;
static bool        m_isAdmin = false;
static int         m_participantCount = 0;
static bool        m_drawDone = false;

// ---------------------------------------------------------------------------

static void refreshCrewStatus()
{
    if (m_inviteCode.empty()) return;
    try {
        json req;
        req["action"] = "GET_CREW_STATUS";
        req["invite_code"] = m_inviteCode;
        json res = sendRequest(req);
        if (res["status"] == "OK") {
            m_participantCount = res["participants_count"].get<int>();
            m_drawDone = res["draw_done"].get<bool>();
        }
    }
    catch (...) {}
}

static void printResponse(const json& res)
{
    std::string status = res.value("status", "ERROR");
    std::cout << (status == "OK" ? "[OK]" : "[ERREUR]") << "\n";

    if (res.contains("message"))
        std::cout << "  " << res["message"].get<std::string>() << "\n";
    if (res.contains("invite_code"))
        std::cout << "  Code crew    : " << res["invite_code"].get<std::string>() << "\n";
    if (res.contains("participants_count"))
        std::cout << "  Participants : " << res["participants_count"] << "\n";
    if (res.contains("name"))
        std::cout << "  Nom          : " << res["name"].get<std::string>() << "\n";
    if (res.contains("email"))
        std::cout << "  Email        : " << res["email"].get<std::string>() << "\n";
    if (res.contains("participants")) {
        std::cout << "  Participants :\n";
        for (const auto& p : res["participants"])
            std::cout << "    - " << p.get<std::string>() << "\n";
    }
    if (res.contains("wishes")) {
        auto& wishes = res["wishes"];
        if (wishes.empty()) {
            std::cout << "  Liste de voeux : (vide)\n";
        }
        else {
            std::cout << "  Liste de voeux :\n";
            int idx = 0;
            for (const auto& w : wishes)
                std::cout << "    [" << idx++ << "] " << w.get<std::string>() << "\n";
        }
    }
    if (res.contains("crews")) {
        auto& crews = res["crews"];
        if (crews.empty()) {
            std::cout << "  Crews : (aucun)\n";
        }
        else {
            std::cout << "  Crews :\n";
            for (const auto& c : crews)
                std::cout << "    - " << c["code"].get<std::string>() << "\n";
        }
    }
}

static json call(const json& req)
{
    try {
        return sendRequest(req);
    }
    catch (const std::exception& e) {
        return json{ {"status","ERROR"},{"message", std::string(e.what())} };
    }
}

// ---------------------------------------------------------------------------
// Profile menu
// ---------------------------------------------------------------------------

static void profileMenu()
{
    while (true)
    {
        std::cout << "\n  Bienvenue sur SecretSanta !\n\n";
        std::cout << "  1. Creer un profil\n";
        std::cout << "  2. Se connecter\n";
        std::cout << "  3. Continuer sans profil\n";
        std::cout << "  Choix : ";

        int choice;
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (choice == 1)
        {
            std::string username, password, name, email;
            std::cout << "Nom d'utilisateur : "; std::getline(std::cin, username);
            std::cout << "Mot de passe      : "; std::getline(std::cin, password);

            if (!m_name.empty()) {
                std::cout << "Votre nom [" << m_name << "] (Entree pour confirmer) : ";
                std::string input; std::getline(std::cin, input);
                name = input.empty() ? m_name : input;
            }
            else {
                std::cout << "Votre nom   : "; std::getline(std::cin, name);
            }

            if (!m_email.empty()) {
                std::cout << "Votre email [" << m_email << "] (Entree pour confirmer) : ";
                std::string input; std::getline(std::cin, input);
                email = input.empty() ? m_email : input;
            }
            else {
                std::cout << "Votre email : "; std::getline(std::cin, email);
            }

            json req;
            req["action"] = "CREATE_PROFILE";
            req["username"] = username;
            req["password"] = password;
            req["name"] = name;
            req["email"] = email;

            json res = call(req);
            printResponse(res);

            if (res["status"] == "OK") {
                m_profileToken = res["profile_token"].get<std::string>();
                m_name = res["name"].get<std::string>();
                m_email = res["email"].get<std::string>();

                if (!m_inviteCode.empty() && !m_crewToken.empty()) {
                    json linkReq;
                    linkReq["action"] = "LINK_CREW_TO_PROFILE";
                    linkReq["profile_token"] = m_profileToken;
                    linkReq["invite_code"] = m_inviteCode;
                    linkReq["crew_token"] = m_crewToken;
                    call(linkReq);
                    std::cout << "  Crew " << m_inviteCode << " lie a votre profil.\n";
                }
                return;
            }
        }
        else if (choice == 2)
        {
            std::string username, password;
            std::cout << "Nom d'utilisateur : "; std::getline(std::cin, username);
            std::cout << "Mot de passe      : "; std::getline(std::cin, password);

            json req;
            req["action"] = "LOGIN_PROFILE";
            req["username"] = username;
            req["password"] = password;

            json res = call(req);
            printResponse(res);

            if (res["status"] == "OK") {
                m_profileToken = res["profile_token"].get<std::string>();
                m_name = res["name"].get<std::string>();
                m_email = res["email"].get<std::string>();

                if (res.contains("crews") && !res["crews"].empty()) {
                    auto& firstCrew = res["crews"][0];
                    m_inviteCode = firstCrew["code"].get<std::string>();
                    m_crewToken = firstCrew["token"].get<std::string>();
                    refreshCrewStatus();
                    std::cout << "  Crew restaure : " << m_inviteCode << "\n";
                }
                return;
            }
        }
        else if (choice == 3)
        {
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// Main menu
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "Genie - santa.colep.fr\n";

    profileMenu();

    bool running = true;
    while (running)
    {
        std::cout << "\n";

        if (!m_name.empty())
            std::cout << "  [" << m_name
            << (!m_inviteCode.empty() ? " | crew " + m_inviteCode : "")
            << (m_isAdmin ? " | admin" : "")
            << "]\n\n";

        // Build menu dynamically with sequential numbers
        int opt = 1;
        std::map<int, std::string> menu;

        if (m_profileToken.empty())
            menu[opt++] = "CREATE_PROFILE_MAIN";
        menu[opt++] = "CREATE_CREW";
        menu[opt++] = "LOGIN_CREW";
        menu[opt++] = "JOIN_CREW";

        if (m_isAdmin && m_participantCount >= 3 && !m_drawDone)
            menu[opt++] = "RUN_DRAW";
        if (m_isAdmin && m_drawDone)
            menu[opt++] = "SEND_EMAILS";
        if (!m_profileToken.empty()) {
            menu[opt++] = "GET_WISHES";
            menu[opt++] = "ADD_WISH";
            menu[opt++] = "REMOVE_WISH";
            menu[opt++] = "GET_PROFILE";
        }
        if (m_isAdmin) {
            menu[opt++] = "REMOVE_PARTICIPANT";
            if (m_drawDone)
                menu[opt++] = "RESET_DRAW";
        }
        int quitOpt = opt;
        menu[opt++] = "QUIT";

        // Print menu
        static const std::map<std::string, std::string> labels = {
            {"CREATE_CREW",        "Creer un crew"},
            {"LOGIN_CREW",         "Se connecter a un crew"},
            {"JOIN_CREW",          "Rejoindre un crew"},
            {"RUN_DRAW",           "Lancer le tirage"},
            {"SEND_EMAILS",        "Envoyer les emails"},
            {"GET_WISHES",         "Voir ma liste de voeux"},
            {"ADD_WISH",           "Ajouter un voeu"},
            {"REMOVE_WISH",        "Supprimer un voeu"},
            {"GET_PROFILE",        "Voir mon profil"},
            {"REMOVE_PARTICIPANT", "Retirer un participant"},
            {"RESET_DRAW",         "Reinitialiser le tirage"},
            {"QUIT",               "Quitter"},
            {"CREATE_PROFILE_MAIN", "Creer / Se connecter a un profil"},
        };

        for (const auto& entry : menu)
            std::cout << "  " << entry.first << ". " << labels.at(entry.second) << "\n";

        std::cout << "  Choix : ";
        int choice;
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (menu.find(choice) == menu.end()) continue;
        std::string action = menu[choice];

        // ── CREATE PROFILE FROM MAIN MENU ───────────────────────────────────
        if (action == "CREATE_PROFILE_MAIN")
        {
            profileMenu();
        }

        // ── QUIT ────────────────────────────────────────────────────────────
        else if (action == "QUIT")
        {
            running = false;
        }

        // ── CREATE CREW ─────────────────────────────────────────────────────
        else if (action == "CREATE_CREW")
        {
            std::string crewName;
            std::cout << "Nom du crew : "; std::getline(std::cin, crewName);

            // Without profile: ask for name and email
            if (m_profileToken.empty() && m_name.empty()) {
                std::cout << "Votre nom   : "; std::getline(std::cin, m_name);
                std::cout << "Votre email : "; std::getline(std::cin, m_email);
            }

            json req;
            req["action"] = "CREATE_CREW";
            req["crew_name"] = crewName;
            if (!m_profileToken.empty())
                req["profile_token"] = m_profileToken;
            else
                req["user"] = { {"name", m_name}, {"email", m_email} };

            json res = call(req);
            printResponse(res);

            if (res["status"] == "OK") {
                m_inviteCode = res["invite_code"].get<std::string>();
                m_crewToken = res["token"].get<std::string>();
                m_isAdmin = true;
                refreshCrewStatus();
            }
        }

        // ── LOGIN CREW ──────────────────────────────────────────────────────
        else if (action == "LOGIN_CREW")
        {
            std::string code;
            std::cout << "Code du crew : "; std::getline(std::cin, code);

            // Without profile: ask for name if not known
            if (m_profileToken.empty() && m_name.empty()) {
                std::cout << "Votre nom   : "; std::getline(std::cin, m_name);
            }

            json req;
            req["action"] = "LOGIN_CREW";
            req["invite_code"] = code;
            if (!m_profileToken.empty())
                req["profile_token"] = m_profileToken;
            else
                req["name"] = m_name;

            json res = call(req);
            printResponse(res);

            if (res["status"] == "OK") {
                m_inviteCode = code;
                m_crewToken = res["token"].get<std::string>();
                m_isAdmin = res["is_owner"].get<bool>();
                refreshCrewStatus();
            }
        }

        // ── JOIN CREW ───────────────────────────────────────────────────────
        else if (action == "JOIN_CREW")
        {
            std::string code;
            std::cout << "Code du crew : "; std::getline(std::cin, code);

            // Without profile: ask for name and email if not known
            if (m_profileToken.empty() && m_name.empty()) {
                std::cout << "Votre nom   : "; std::getline(std::cin, m_name);
                std::cout << "Votre email : "; std::getline(std::cin, m_email);
            }

            json req;
            req["action"] = "JOIN_CREW";
            req["invite_code"] = code;
            if (!m_profileToken.empty())
                req["profile_token"] = m_profileToken;
            else
                req["user"] = { {"name", m_name}, {"email", m_email} };

            json res = call(req);
            printResponse(res);

            if (res["status"] == "OK") {
                m_inviteCode = code;
                m_crewToken = res["token"].get<std::string>();
                m_isAdmin = false;
                refreshCrewStatus();
            }
        }

        // ── RUN DRAW ────────────────────────────────────────────────────────
        else if (action == "RUN_DRAW")
        {
            json req;
            req["action"] = "RUN_DRAW";
            req["invite_code"] = m_inviteCode;
            req["admin_token"] = ADMIN_TOKEN;
            json res = call(req);
            printResponse(res);
            if (res["status"] == "OK") refreshCrewStatus();
        }

        // ── SEND EMAILS ─────────────────────────────────────────────────────
        else if (action == "SEND_EMAILS")
        {
            json req;
            req["action"] = "SEND_EMAILS";
            req["invite_code"] = m_inviteCode;
            req["admin_token"] = ADMIN_TOKEN;
            printResponse(call(req));
        }

        // ── GET WISHES ──────────────────────────────────────────────────────
        else if (action == "GET_WISHES")
        {
            json req;
            req["action"] = "GET_WISHES";
            req["profile_token"] = m_profileToken;
            printResponse(call(req));
        }

        // ── ADD WISH ────────────────────────────────────────────────────────
        else if (action == "ADD_WISH")
        {
            std::string wish;
            std::cout << "Votre voeu : "; std::getline(std::cin, wish);

            json req;
            req["action"] = "ADD_WISH";
            req["profile_token"] = m_profileToken;
            req["wish"] = wish;
            printResponse(call(req));
        }

        // ── REMOVE WISH ─────────────────────────────────────────────────────
        else if (action == "REMOVE_WISH")
        {
            json getReq;
            getReq["action"] = "GET_WISHES";
            getReq["profile_token"] = m_profileToken;
            json getRes = call(getReq);
            printResponse(getRes);

            if (!getRes.contains("wishes") || getRes["wishes"].empty()) {
                std::cout << "  Aucun voeu a supprimer.\n"; continue;
            }

            int index;
            std::cout << "Index a supprimer : ";
            std::cin >> index;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            json req;
            req["action"] = "REMOVE_WISH";
            req["profile_token"] = m_profileToken;
            req["index"] = index;
            printResponse(call(req));
        }

        // ── GET PROFILE ─────────────────────────────────────────────────────
        else if (action == "GET_PROFILE")
        {
            json req;
            req["action"] = "GET_PROFILE";
            req["profile_token"] = m_profileToken;
            printResponse(call(req));
        }

        // ── REMOVE PARTICIPANT ───────────────────────────────────────────────
        else if (action == "REMOVE_PARTICIPANT")
        {
            json listReq;
            listReq["action"] = "GET_PARTICIPANTS";
            listReq["invite_code"] = m_inviteCode;
            printResponse(call(listReq));

            std::string name;
            std::cout << "Nom du participant a retirer : ";
            std::getline(std::cin, name);

            json req;
            req["action"] = "REMOVE_PARTICIPANT";
            req["invite_code"] = m_inviteCode;
            req["admin_token"] = ADMIN_TOKEN;
            req["name"] = name;

            json res = call(req);
            printResponse(res);
            if (res["status"] == "OK") refreshCrewStatus();
        }

        // ── RESET DRAW ───────────────────────────────────────────────────────
        else if (action == "RESET_DRAW")
        {
            std::cout << "  Attention : les emails ont peut-etre deja ete envoyes. Continuer ? (o/n) : ";
            char c; std::cin >> c;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            if (c != 'o' && c != 'O') continue;

            json req;
            req["action"] = "RESET_DRAW";
            req["invite_code"] = m_inviteCode;
            req["admin_token"] = ADMIN_TOKEN;

            json res = call(req);
            printResponse(res);
            if (res["status"] == "OK") refreshCrewStatus();
        }
    }

    return 0;
}