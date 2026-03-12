#define NOMINMAX
#include <iostream>
#include <limits>
#include <string>
#include "NetworkUtils.h"
#include "../external/json.hpp"

using json = nlohmann::json;

static const std::string ADMIN_TOKEN = "SANTA-ADMIN-I-KNOW-COLEP-972-/-MATHIEU";

// Session state
static std::string m_inviteCode;
static std::string m_token;
static std::string m_name;
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
    if (res.contains("token"))
        std::cout << "  Votre token  : " << res["token"].get<std::string>() << "  (notez-le !)\n";
    if (res.contains("participants_count"))
        std::cout << "  Participants : " << res["participants_count"] << "\n";
    if (res.contains("name"))
        std::cout << "  Nom          : " << res["name"].get<std::string>() << "\n";
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

int main()
{
    std::cout << "SecretSanta - connexion a santa.colep.fr\n";

    bool running = true;
    while (running)
    {
        std::cout << "\n";
        if (!m_inviteCode.empty())
            std::cout << "  [" << m_name << " | crew " << m_inviteCode
            << (m_isAdmin ? " | admin" : "") << "]\n\n";

        std::cout << "  1.  Creer un crew\n";
        std::cout << "  2.  Se connecter a un crew (login)\n";
        std::cout << "  3.  Rejoindre un crew\n";

        if (m_isAdmin && m_participantCount >= 3 && !m_drawDone)
            std::cout << "  4.  Lancer le tirage\n";
        if (m_isAdmin && m_drawDone)
            std::cout << "  5.  Envoyer les emails\n";
        if (!m_inviteCode.empty()) {
            std::cout << "  6.  Voir ma liste de voeux\n";
            std::cout << "  7.  Ajouter un voeu\n";
            std::cout << "  8.  Supprimer un voeu\n";
        }
        if (m_isAdmin) {
            std::cout << "  10. Retirer un participant\n";
            if (m_drawDone)
                std::cout << "  11. Reinitialiser le tirage\n";
        }
        std::cout << "  9.  Quitter\n";
        std::cout << "  Choix : ";

        int choice;
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        // ── 1. CREATE CREW ──────────────────────────────────────────────────
        if (choice == 1)
        {
            std::string crewName, name, email;
            std::cout << "Nom du crew : "; std::getline(std::cin, crewName);
            std::cout << "Ton nom     : "; std::getline(std::cin, name);
            std::cout << "Ton email   : "; std::getline(std::cin, email);

            json req;
            req["action"] = "CREATE_CREW";
            req["crew_name"] = crewName;
            req["user"]["name"] = name;
            req["user"]["email"] = email;

            json res = call(req);
            printResponse(res);

            if (res["status"] == "OK") {
                m_inviteCode = res["invite_code"].get<std::string>();
                m_token = res["token"].get<std::string>();
                m_name = name;
                m_isAdmin = true;
                refreshCrewStatus();
            }
        }

        // ── 2. LOGIN ────────────────────────────────────────────────────────
        else if (choice == 2)
        {
            std::string code, name;
            std::cout << "Code du crew : "; std::getline(std::cin, code);
            std::cout << "Ton nom      : "; std::getline(std::cin, name);

            json req;
            req["action"] = "LOGIN_CREW";
            req["invite_code"] = code;
            req["name"] = name;

            json res = call(req);
            printResponse(res);

            if (res["status"] == "OK") {
                m_inviteCode = code;
                m_token = res["token"].get<std::string>();
                m_name = name;
                m_isAdmin = res["is_owner"].get<bool>();
                refreshCrewStatus();
            }
        }

        // ── 3. JOIN CREW ────────────────────────────────────────────────────
        else if (choice == 3)
        {
            std::string code, name, email;
            std::cout << "Code du crew : "; std::getline(std::cin, code);
            std::cout << "Ton nom      : "; std::getline(std::cin, name);
            std::cout << "Ton email    : "; std::getline(std::cin, email);

            json req;
            req["action"] = "JOIN_CREW";
            req["invite_code"] = code;
            req["user"]["name"] = name;
            req["user"]["email"] = email;

            json res = call(req);
            printResponse(res);

            if (res["status"] == "OK") {
                m_inviteCode = code;
                m_token = res["token"].get<std::string>();
                m_name = name;
                m_isAdmin = false;
                refreshCrewStatus();
            }
        }

        // ── 4. RUN DRAW ─────────────────────────────────────────────────────
        else if (choice == 4)
        {
            json req;
            req["action"] = "RUN_DRAW";
            req["invite_code"] = m_inviteCode;
            req["admin_token"] = ADMIN_TOKEN;

            json res = call(req);
            printResponse(res);
            if (res["status"] == "OK") refreshCrewStatus();
        }

        // ── 5. SEND EMAILS ──────────────────────────────────────────────────
        else if (choice == 5)
        {
            json req;
            req["action"] = "SEND_EMAILS";
            req["invite_code"] = m_inviteCode;
            req["admin_token"] = ADMIN_TOKEN;

            json res = call(req);
            printResponse(res);
        }

        // ── 6. VIEW WISHES ──────────────────────────────────────────────────
        else if (choice == 6)
        {
            json req;
            req["action"] = "GET_WISHES";
            req["invite_code"] = m_inviteCode;
            req["token"] = m_token;

            printResponse(call(req));
        }

        // ── 7. ADD WISH ─────────────────────────────────────────────────────
        else if (choice == 7)
        {
            std::string wish;
            std::cout << "Votre voeu : "; std::getline(std::cin, wish);

            json req;
            req["action"] = "ADD_WISH";
            req["invite_code"] = m_inviteCode;
            req["token"] = m_token;
            req["wish"] = wish;

            printResponse(call(req));
        }

        // ── 8. REMOVE WISH ──────────────────────────────────────────────────
        else if (choice == 8)
        {
            json getReq;
            getReq["action"] = "GET_WISHES";
            getReq["invite_code"] = m_inviteCode;
            getReq["token"] = m_token;
            json getRes = call(getReq);
            printResponse(getRes);

            if (!getRes.contains("wishes") || getRes["wishes"].empty()) {
                std::cout << "  Aucun voeu a supprimer.\n";
                continue;
            }

            int index;
            std::cout << "Index a supprimer : ";
            std::cin >> index;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            json req;
            req["action"] = "REMOVE_WISH";
            req["invite_code"] = m_inviteCode;
            req["token"] = m_token;
            req["index"] = index;

            printResponse(call(req));
        }

        // ── 9. QUIT ─────────────────────────────────────────────────────────
        else if (choice == 9)
        {
            running = false;
        }

        // ── 10. REMOVE PARTICIPANT ───────────────────────────────────────────
        else if (choice == 10)
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

        // ── 11. RESET DRAW ───────────────────────────────────────────────────
        else if (choice == 11)
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