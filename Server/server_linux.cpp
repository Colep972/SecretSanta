#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include "Crew.h"
#include "Users.h"
#include "Save.h"
#include "Draw.h"
#include "Mailer.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <random>
#include <string>
#include <map>

using json = nlohmann::json;

static const std::string DATA_DIR = "ServerData/";
static const std::string CREWS_DIR = "ServerData/Crews/";
static const std::string INVITES_FILE = DATA_DIR + "invites.json";
static const std::string ADMIN_TOKEN = "SANTA-ADMIN-I-KNOW-COLEP-972-/-MATHIEU";

static std::string generateToken()
{
    static const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    static std::mt19937 rng(std::random_device{}());
    static std::uniform_int_distribution<int> dist(0, sizeof(charset) - 2);
    std::string token(16, ' ');
    for (auto& c : token) c = charset[dist(rng)];
    return token;
}

static std::string generateInviteCode()
{
    static const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static std::mt19937 rng(std::random_device{}());
    static std::uniform_int_distribution<int> dist(0, sizeof(charset) - 2);
    std::string code = "SANTA-";
    for (int i = 0; i < 6; ++i) code += charset[dist(rng)];
    return code;
}

static std::string crewFile(const std::string& code)
{
    return CREWS_DIR + code + ".json";
}

static std::map<std::string, std::string> loadInvites()
{
    std::map<std::string, std::string> m;
    std::ifstream f(INVITES_FILE);
    if (!f.is_open()) return m;
    json j; f >> j;
    for (auto it = j.begin(); it != j.end(); ++it)
        m[it.key()] = it.value().get<std::string>();
    return m;
}

static void saveInvites(const std::map<std::string, std::string>& m)
{
    json j = m;
    std::ofstream f(INVITES_FILE);
    f << j.dump(4);
}

static json ok(json body = {}) { body["status"] = "OK";    return body; }
static json err(const std::string& msg) { return json{ {"status","ERROR"},{"message",msg} }; }

static json handleRequest(const json& req)
{
    if (!req.contains("action")) return err("Missing action");
    std::string action = req["action"].get<std::string>();

    if (action == "PING")
        return ok({ {"message","pong"} });

    else if (action == "CREATE_CREW")
    {
        if (!req.contains("crew_name") || !req.contains("user"))
            return err("Missing crew_name or user");
        std::string crewName = req["crew_name"].get<std::string>();
        std::string name = req["user"]["name"].get<std::string>();
        std::string email = req["user"]["email"].get<std::string>();
        std::string code = generateInviteCode();
        std::string token = generateToken();
        Users owner(name, email);
        owner.setToken(token);
        Crew crew(crewName);
        crew.setOwnerToken(token);
        crew.addUser(owner);
        std::filesystem::create_directories(CREWS_DIR);
        Save::saveCrew(crew, crewFile(code));
        auto invites = loadInvites();
        invites[code] = crewName;
        saveInvites(invites);
        return ok({ {"invite_code",code},{"token",token},{"is_owner",true} });
    }

    else if (action == "JOIN_CREW")
    {
        if (!req.contains("invite_code") || !req.contains("user"))
            return err("Missing invite_code or user");
        std::string code = req["invite_code"].get<std::string>();
        std::string name = req["user"]["name"].get<std::string>();
        std::string email = req["user"]["email"].get<std::string>();
        Crew crew("");
        if (!Save::loadCrew(crew, crewFile(code))) return err("Crew not found");
        std::string token = generateToken();
        Users newUser(name, email);
        newUser.setToken(token);
        crew.addUser(newUser);
        Save::saveCrew(crew, crewFile(code));
        return ok({ {"token",token},{"is_owner",false} });
    }

    else if (action == "LOGIN_CREW")
    {
        if (!req.contains("invite_code") || !req.contains("name"))
            return err("Missing invite_code or name");
        std::string code = req["invite_code"].get<std::string>();
        std::string name = req["name"].get<std::string>();
        Crew crew("");
        if (!Save::loadCrew(crew, crewFile(code))) return err("Crew not found");
        Users* user = crew.findUserByName(name);
        if (!user) return err("User not found");
        bool isOwner = (user->getToken() == crew.getOwnerToken());
        return ok({ {"token",user->getToken()},{"is_owner",isOwner} });
    }

    else if (action == "GET_CREW_STATUS")
    {
        if (!req.contains("invite_code")) return err("Missing invite_code");
        std::string code = req["invite_code"].get<std::string>();
        Crew crew("");
        if (!Save::loadCrew(crew, crewFile(code))) return err("Crew not found");
        std::map<std::string, std::string> draw;
        bool drawDone = Save::loadDrawResult(crewFile(code), draw);
        return ok({ {"participants_count",(int)crew.getUsers().size()},{"draw_done",drawDone} });
    }

    else if (action == "GET_PARTICIPANTS")
    {
        if (!req.contains("invite_code")) return err("Missing invite_code");
        std::string code = req["invite_code"].get<std::string>();
        Crew crew("");
        if (!Save::loadCrew(crew, crewFile(code))) return err("Crew not found");
        json names = json::array();
        for (const auto& u : crew.getUsers()) names.push_back(u.getName());
        return ok({ {"participants",names} });
    }

    else if (action == "RUN_DRAW")
    {
        if (!req.contains("invite_code") || !req.contains("admin_token"))
            return err("Missing fields");
        if (req["admin_token"].get<std::string>() != ADMIN_TOKEN)
            return err("Invalid admin token");
        std::string code = req["invite_code"].get<std::string>();
        Crew crew("");
        if (!Save::loadCrew(crew, crewFile(code))) return err("Crew not found");
        if (crew.getUsers().size() < 3) return err("At least 3 participants required");
        auto results = Draw::run(crew);
        Save::saveDrawResult(crewFile(code), results);
        return ok({ {"message","Draw done"} });
    }

    else if (action == "SEND_EMAILS")
    {
        if (!req.contains("invite_code") || !req.contains("admin_token"))
            return err("Missing fields");
        if (req["admin_token"].get<std::string>() != ADMIN_TOKEN)
            return err("Invalid admin token");
        std::string code = req["invite_code"].get<std::string>();
        Crew crew("");
        if (!Save::loadCrew(crew, crewFile(code))) return err("Crew not found");
        std::map<std::string, std::string> draw;
        if (!Save::loadDrawResult(crewFile(code), draw)) return err("Draw not done yet");
        Mailer::send(crew, draw);
        return ok({ {"message","Emails sent"} });
    }

    else if (action == "ADD_WISH")
    {
        if (!req.contains("invite_code") || !req.contains("token") || !req.contains("wish"))
            return err("Missing fields");
        std::string code = req["invite_code"].get<std::string>();
        std::string token = req["token"].get<std::string>();
        std::string wish = req["wish"].get<std::string>();
        Crew crew("");
        if (!Save::loadCrew(crew, crewFile(code))) return err("Crew not found");
        Users* user = crew.findUserByToken(token);
        if (!user) return err("Invalid token");
        user->addWish(wish);
        Save::saveCrew(crew, crewFile(code));
        return ok({ {"message","Wish added"} });
    }

    else if (action == "REMOVE_WISH")
    {
        if (!req.contains("invite_code") || !req.contains("token") || !req.contains("index"))
            return err("Missing fields");
        std::string code = req["invite_code"].get<std::string>();
        std::string token = req["token"].get<std::string>();
        int index = req["index"].get<int>();
        Crew crew("");
        if (!Save::loadCrew(crew, crewFile(code))) return err("Crew not found");
        Users* user = crew.findUserByToken(token);
        if (!user) return err("Invalid token");
        if (index < 0 || index >= (int)user->getWishes().size()) return err("Invalid index");
        user->removeWish(index);
        Save::saveCrew(crew, crewFile(code));
        return ok({ {"message","Wish removed"} });
    }

    else if (action == "GET_WISHES")
    {
        if (!req.contains("invite_code") || !req.contains("token"))
            return err("Missing fields");
        std::string code = req["invite_code"].get<std::string>();
        std::string token = req["token"].get<std::string>();
        Crew crew("");
        if (!Save::loadCrew(crew, crewFile(code))) return err("Crew not found");
        Users* user = crew.findUserByToken(token);
        if (!user) return err("Invalid token");
        return ok({ {"name",user->getName()},{"wishes",user->getWishes()} });
    }

    else if (action == "REMOVE_PARTICIPANT")
    {
        if (!req.contains("invite_code") || !req.contains("admin_token") || !req.contains("name"))
            return err("Missing fields");
        if (req["admin_token"].get<std::string>() != ADMIN_TOKEN)
            return err("Invalid admin token");
        std::string code = req["invite_code"].get<std::string>();
        std::string name = req["name"].get<std::string>();
        Crew crew("");
        if (!Save::loadCrew(crew, crewFile(code))) return err("Crew not found");
        std::map<std::string, std::string> draw;
        if (Save::loadDrawResult(crewFile(code), draw)) return err("Draw already done. Reset it first.");
        Users* target = crew.findUserByName(name);
        if (!target) return err("User not found");
        if (target->getToken() == crew.getOwnerToken()) return err("Cannot remove the crew owner");
        crew.removeUser(name);
        Save::saveCrew(crew, crewFile(code));
        return ok({ {"message","Participant removed"} });
    }

    else if (action == "RESET_DRAW")
    {
        if (!req.contains("invite_code") || !req.contains("admin_token"))
            return err("Missing fields");
        if (req["admin_token"].get<std::string>() != ADMIN_TOKEN)
            return err("Invalid admin token");
        std::string code = req["invite_code"].get<std::string>();
        if (!Save::clearDrawResult(crewFile(code))) return err("Failed to reset draw");
        return ok({ {"message","Draw reset"} });
    }

    return err("Unknown action: " + action);
}

int main()
{
    std::filesystem::create_directories(CREWS_DIR);

    httplib::SSLServer svr(
        "/etc/letsencrypt/live/santa.colep.fr/fullchain.pem",
        "/etc/letsencrypt/live/santa.colep.fr/privkey.pem"
    );

    svr.Post("/api", [](const httplib::Request& req, httplib::Response& res) {
        json response;
        try {
            json request = json::parse(req.body);
            response = handleRequest(request);
        }
        catch (const std::exception& e) {
            response = err(std::string("Parse error: ") + e.what());
        }
        res.set_content(response.dump(), "application/json");
        });

    std::cout << "SecretSanta HTTPS server on port 443..." << std::endl;
    svr.listen("0.0.0.0", 443);
    return 0;
}