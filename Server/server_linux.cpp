#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include "Crew.h"
#include "Users.h"
#include "Save.h"
#include "Draw.h"
#include "Mailer.h"
#include "Profile.h"
#include "Pot.h"
#include "ChatServer.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <random>
#include <string>
#include <map>
#include <functional>

using json = nlohmann::json;

static const std::string DATA_DIR = "ServerData/";
static const std::string CREWS_DIR = "ServerData/Crews/";
static const std::string PROFILES_DIR = "ServerData/Profiles/";
static const std::string INVITES_FILE = DATA_DIR + "invites.json";
static const std::string ADMIN_TOKEN = "GENIE-ADMIN-I-KNOW-COLEP-972-/-MATHIEU";
static const std::string APP_VERSION = "1.0.2";
static const std::string CLIENT_EXE = "Client.exe";

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

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
    std::string code = "GENIE-";
    for (int i = 0; i < 6; ++i) code += charset[dist(rng)];
    return code;
}

static std::string hashPassword(const std::string& password)
{
    // Simple hash using std::hash — good enough for this app
    std::size_t h = std::hash<std::string>{}(password + "secretsanta_salt");
    return std::to_string(h);
}

static std::string crewFile(const std::string& code) { return CREWS_DIR + code + ".json"; }
static std::string potFile(const std::string& code) { return CREWS_DIR + code + "_pots.json"; }
static std::string profileFile(const std::string& u) { return PROFILES_DIR + u + ".json"; }

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

// Active profile sessions: token -> username
static std::map<std::string, std::string> g_profileSessions;

static Profile* getProfileByToken(const std::string& token, Profile& out)
{
    auto it = g_profileSessions.find(token);
    if (it == g_profileSessions.end()) return nullptr;
    if (!Save::loadProfile(out, profileFile(it->second))) return nullptr;
    return &out;
}

static json ok(json body = {}) { body["status"] = "OK";    return body; }
static json err(const std::string& msg) { return json{ {"status","ERROR"},{"message",msg} }; }

// ---------------------------------------------------------------------------
// Request handler
// ---------------------------------------------------------------------------

static json handleRequest(const json& req)
{
    if (!req.contains("action")) return err("Missing action");
    std::string action = req["action"].get<std::string>();

    // ── PING ────────────────────────────────────────────────────────────────
    if (action == "PING")
        return ok({ {"message","pong"} });

    // ── GET_VERSION ──────────────────────────────────────────────────────────
    else if (action == "GET_VERSION")
        return ok({ {"version", APP_VERSION} });

    // ── CREATE_PROFILE ───────────────────────────────────────────────────────
    else if (action == "CREATE_PROFILE")
    {
        if (!req.contains("username") || !req.contains("password") ||
            !req.contains("name") || !req.contains("email"))
            return err("Missing fields");

        std::string username = req["username"].get<std::string>();
        std::string password = req["password"].get<std::string>();
        std::string name = req["name"].get<std::string>();
        std::string email = req["email"].get<std::string>();

        std::filesystem::create_directories(PROFILES_DIR);

        if (std::filesystem::exists(profileFile(username)))
            return err("Username already taken");

        Profile profile(username, hashPassword(password), name, email);
        Save::saveProfile(profile, profileFile(username));

        std::string token = generateToken();
        g_profileSessions[token] = username;

        return ok({ {"profile_token", token}, {"name", name}, {"email", email} });
    }

    // ── LOGIN_PROFILE ────────────────────────────────────────────────────────
    else if (action == "LOGIN_PROFILE")
    {
        if (!req.contains("username") || !req.contains("password"))
            return err("Missing fields");

        std::string username = req["username"].get<std::string>();
        std::string password = req["password"].get<std::string>();

        Profile profile;
        if (!Save::loadProfile(profile, profileFile(username)))
            return err("User not found");

        if (profile.getPasswordHash() != hashPassword(password))
            return err("Invalid password");

        std::string token = generateToken();
        g_profileSessions[token] = username;

        json crews = json::array();
        for (const auto& c : profile.getCrews())
            crews.push_back({ {"code", c.code}, {"token", c.token} });

        return ok({
            {"profile_token", token},
            {"name", profile.getName()},
            {"email", profile.getEmail()},
            {"wishes", profile.getWishes()},
            {"crews", crews}
            });
    }

    // ── GET_PROFILE ──────────────────────────────────────────────────────────
    else if (action == "GET_PROFILE")
    {
        if (!req.contains("profile_token")) return err("Missing profile_token");
        Profile profile;
        if (!getProfileByToken(req["profile_token"].get<std::string>(), profile))
            return err("Invalid profile token");

        json crews = json::array();
        for (const auto& c : profile.getCrews())
            crews.push_back({ {"code", c.code}, {"token", c.token} });

        return ok({
            {"name", profile.getName()},
            {"email", profile.getEmail()},
            {"wishes", profile.getWishes()},
            {"crews", crews}
            });
    }

    // ── LINK_CREW_TO_PROFILE ─────────────────────────────────────────────────
    else if (action == "LINK_CREW_TO_PROFILE")
    {
        if (!req.contains("profile_token") || !req.contains("invite_code") ||
            !req.contains("crew_token"))
            return err("Missing fields");

        std::string profileToken = req["profile_token"].get<std::string>();
        std::string code = req["invite_code"].get<std::string>();
        std::string crewToken = req["crew_token"].get<std::string>();

        Profile profile;
        if (!getProfileByToken(profileToken, profile))
            return err("Invalid profile token");

        profile.addCrew(code, crewToken);
        Save::saveProfile(profile, profileFile(profile.getUsername()));

        return ok({ {"message", "Crew linked to profile"} });
    }

    // ── CREATE_CREW ──────────────────────────────────────────────────────────
    else if (action == "CREATE_CREW")
    {
        if (!req.contains("crew_name"))
            return err("Missing crew_name");

        std::string crewName = req["crew_name"].get<std::string>();
        std::string code = generateInviteCode();
        std::string token = generateToken();
        std::string name, email;

        // With profile
        if (req.contains("profile_token"))
        {
            Profile profile;
            if (!getProfileByToken(req["profile_token"].get<std::string>(), profile))
                return err("Invalid profile token");
            name = profile.getName();
            email = profile.getEmail();
            profile.addCrew(code, token);
            Save::saveProfile(profile, profileFile(profile.getUsername()));
        }
        else
        {
            // Without profile — need user field
            if (!req.contains("user"))
                return err("Missing profile_token or user");
            name = req["user"]["name"].get<std::string>();
            email = req["user"]["email"].get<std::string>();
        }

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

    // ── JOIN_CREW ────────────────────────────────────────────────────────────
    else if (action == "JOIN_CREW")
    {
        if (!req.contains("invite_code"))
            return err("Missing invite_code");

        std::string code = req["invite_code"].get<std::string>();
        std::string name, email;

        Crew crew("");
        if (!Save::loadCrew(crew, crewFile(code))) return err("Crew not found");

        // With profile
        if (req.contains("profile_token"))
        {
            Profile profile;
            if (!getProfileByToken(req["profile_token"].get<std::string>(), profile))
                return err("Invalid profile token");
            name = profile.getName();
            email = profile.getEmail();

            if (crew.findUserByName(name))
                return err("Already in this crew");

            std::string token = generateToken();
            Users newUser(name, email);
            newUser.setToken(token);
            crew.addUser(newUser);
            Save::saveCrew(crew, crewFile(code));

            profile.addCrew(code, token);
            Save::saveProfile(profile, profileFile(profile.getUsername()));

            return ok({ {"token",token},{"is_owner",false} });
        }
        else
        {
            // Without profile — need user field
            if (!req.contains("user"))
                return err("Missing profile_token or user");
            name = req["user"]["name"].get<std::string>();
            email = req["user"]["email"].get<std::string>();

            if (crew.findUserByName(name))
                return err("Already in this crew");

            std::string token = generateToken();
            Users newUser(name, email);
            newUser.setToken(token);
            crew.addUser(newUser);
            Save::saveCrew(crew, crewFile(code));

            return ok({ {"token",token},{"is_owner",false} });
        }
    }

    // ── LOGIN_CREW ───────────────────────────────────────────────────────────
    else if (action == "LOGIN_CREW")
    {
        if (!req.contains("invite_code"))
            return err("Missing invite_code");

        std::string code = req["invite_code"].get<std::string>();
        std::string name;

        if (req.contains("profile_token"))
        {
            Profile profile;
            if (!getProfileByToken(req["profile_token"].get<std::string>(), profile))
                return err("Invalid profile token");
            name = profile.getName();
        }
        else if (req.contains("name"))
        {
            name = req["name"].get<std::string>();
        }
        else
        {
            return err("Missing profile_token or name");
        }

        Crew crew("");
        if (!Save::loadCrew(crew, crewFile(code))) return err("Crew not found");

        const Users* user = crew.findUserByName(name);
        if (!user) return err("Not a member of this crew");

        bool isOwner = (user->getToken() == crew.getOwnerToken());
        return ok({ {"token", user->getToken()}, {"is_owner", isOwner} });
    }

    // ── GET_CREW_STATUS ──────────────────────────────────────────────────────
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

    // ── GET_PARTICIPANTS ─────────────────────────────────────────────────────
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

    // ── RUN_DRAW ─────────────────────────────────────────────────────────────
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
        Draw draw;
        if (!draw.run(crew)) return err("Draw failed");
        Save::saveDrawResult(crewFile(code), draw.getResults());
        return ok({ {"message","Draw done"} });
    }

    // ── SEND_EMAILS ──────────────────────────────────────────────────────────
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

        // Inject profile wishes into crew users before sending emails
        for (auto& user : crew.getUsers())
        {
            // Find profile matching this user's name
            for (const auto& entry : std::filesystem::directory_iterator(PROFILES_DIR))
            {
                Profile p;
                if (Save::loadProfile(p, entry.path().string()))
                {
                    if (p.getName() == user.getName())
                    {
                        user.clearWishes();
                        for (const auto& w : p.getWishes())
                            user.addWish(w);
                        break;
                    }
                }
            }
        }

        Mailer mailer;
        mailer.send(crew, draw);
        return ok({ {"message","Emails sent"} });
    }

    // ── ADD_WISH ─────────────────────────────────────────────────────────────
    else if (action == "ADD_WISH")
    {
        if (!req.contains("profile_token") || !req.contains("wish"))
            return err("Missing fields");

        Profile profile;
        if (!getProfileByToken(req["profile_token"].get<std::string>(), profile))
            return err("Invalid profile token");

        profile.addWish(req["wish"].get<std::string>());
        Save::saveProfile(profile, profileFile(profile.getUsername()));
        return ok({ {"message","Wish added"} });
    }

    // ── REMOVE_WISH ──────────────────────────────────────────────────────────
    else if (action == "REMOVE_WISH")
    {
        if (!req.contains("profile_token") || !req.contains("index"))
            return err("Missing fields");

        Profile profile;
        if (!getProfileByToken(req["profile_token"].get<std::string>(), profile))
            return err("Invalid profile token");

        int index = req["index"].get<int>();
        if (index < 0 || index >= (int)profile.getWishes().size())
            return err("Invalid index");

        profile.removeWish(index);
        Save::saveProfile(profile, profileFile(profile.getUsername()));
        return ok({ {"message","Wish removed"} });
    }

    // ── GET_WISHES ───────────────────────────────────────────────────────────
    else if (action == "GET_WISHES")
    {
        if (!req.contains("profile_token"))
            return err("Missing profile_token");

        Profile profile;
        if (!getProfileByToken(req["profile_token"].get<std::string>(), profile))
            return err("Invalid profile token");

        return ok({ {"name", profile.getName()}, {"wishes", profile.getWishes()} });
    }

    // ── REMOVE_PARTICIPANT ───────────────────────────────────────────────────
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
        const Users* target = crew.findUserByName(name);
        if (!target) return err("User not found");
        if (target->getToken() == crew.getOwnerToken()) return err("Cannot remove the crew owner");
        crew.removeUser(name);
        Save::saveCrew(crew, crewFile(code));
        return ok({ {"message","Participant removed"} });
    }

    // ── RESET_DRAW ───────────────────────────────────────────────────────────
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

    // ── CREATE_POT ───────────────────────────────────────────────────────────
    else if (action == "CREATE_POT")
    {
        if (!req.contains("invite_code") || !req.contains("admin_token") ||
            !req.contains("pot_name") || !req.contains("amount"))
            return err("Missing fields");
        if (req["admin_token"].get<std::string>() != ADMIN_TOKEN)
            return err("Invalid admin token");

        std::string code = req["invite_code"].get<std::string>();
        std::string potName = req["pot_name"].get<std::string>();
        double amount = req["amount"].get<double>();

        Crew crew("");
        if (!Save::loadCrew(crew, crewFile(code))) return err("Crew not found");

        std::string creator;
        if (req.contains("profile_token"))
        {
            Profile profile;
            if (!getProfileByToken(req["profile_token"].get<std::string>(), profile))
                return err("Invalid profile token");
            creator = profile.getName();
        }
        else if (req.contains("creator"))
            creator = req["creator"].get<std::string>();
        else
            return err("Missing profile_token or creator");

        std::string potId = generateToken();
        Pot pot(potId, potName, amount, creator);
        for (const auto& user : crew.getUsers())
            pot.addParticipant(user.getName());

        std::vector<Pot> pots;
        Save::loadPots(pots, potFile(code));
        pots.push_back(pot);
        Save::savePots(pots, potFile(code));

        return ok({ {"pot_id", potId}, {"message", "Pot cree"} });
    }

    // ── GET_POTS ─────────────────────────────────────────────────────────────
    else if (action == "GET_POTS")
    {
        if (!req.contains("invite_code")) return err("Missing invite_code");
        std::string code = req["invite_code"].get<std::string>();

        std::vector<Pot> pots;
        Save::loadPots(pots, potFile(code));

        json jpots = json::array();
        for (const auto& pot : pots)
        {
            json jp;
            jp["id"] = pot.getId();
            jp["name"] = pot.getName();
            jp["amount"] = pot.getAmount();
            jp["creator"] = pot.getCreator();
            jp["participants"] = json::array();
            for (const auto& p : pot.getParticipants())
                jp["participants"].push_back({ {"name", p.name}, {"paid", p.paid} });
            jpots.push_back(jp);
        }
        return ok({ {"pots", jpots} });
    }

    // ── MARK_PAID ────────────────────────────────────────────────────────────
    else if (action == "MARK_PAID")
    {
        if (!req.contains("invite_code") || !req.contains("pot_id"))
            return err("Missing fields");

        std::string code = req["invite_code"].get<std::string>();
        std::string potId = req["pot_id"].get<std::string>();
        std::string name;

        if (req.contains("profile_token"))
        {
            Profile profile;
            if (!getProfileByToken(req["profile_token"].get<std::string>(), profile))
                return err("Invalid profile token");
            name = profile.getName();
        }
        else if (req.contains("name"))
            name = req["name"].get<std::string>();
        else
            return err("Missing profile_token or name");

        std::vector<Pot> pots;
        if (!Save::loadPots(pots, potFile(code))) return err("No pots found");

        for (auto& pot : pots)
        {
            if (pot.getId() == potId)
            {
                if (!pot.hasMember(name)) return err("Not a participant of this pot");
                if (pot.hasPaid(name))    return err("Already marked as paid");
                pot.markPaid(name);
                Save::savePots(pots, potFile(code));
                return ok({ {"message", "Marque comme paye"} });
            }
        }
        return err("Pot not found");
    }

    return err("Unknown action: " + action);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main()
{
    std::filesystem::create_directories(CREWS_DIR);
    std::filesystem::create_directories(PROFILES_DIR);

    httplib::SSLServer svr(
        "/etc/letsencrypt/live/santa.colep.fr/fullchain.pem",
        "/etc/letsencrypt/live/santa.colep.fr/privkey.pem"
    );

    // Start WebSocket chat server
    ChatServer chatServer(8445, CREWS_DIR);
    chatServer.start();

    svr.Get("/version", [](const httplib::Request&, httplib::Response& res) {
        json j;
        j["version"] = APP_VERSION;
        res.set_content(j.dump(), "application/json");
        });

    svr.Get("/download", [](const httplib::Request&, httplib::Response& res) {
        std::ifstream file(CLIENT_EXE, std::ios::binary);
        if (!file.is_open()) {
            res.status = 404;
            res.set_content("Not found", "text/plain");
            return;
        }
        std::string content((std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());
        res.set_content(content, "application/octet-stream");
        res.set_header("Content-Disposition", "attachment; filename=Client.exe");
        });

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

    std::cout << "Genie HTTPS server on port 8443..." << std::endl;
    svr.listen("0.0.0.0", 8443);
    return 0;
}