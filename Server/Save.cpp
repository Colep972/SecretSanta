#include "Save.h"
#include <fstream>
#include <iostream>
#include "../external/json.hpp"

using json = nlohmann::json;

//bool Save::saveProfile(const Profile& profile, const std::string& filename)
//{
//    json j;
//    j["username"] = profile.getUsername();
//    j["crews"] = profile.getCrewFiles();
//
//    std::ofstream file(filename);
//    if (!file.is_open())
//    {
//        std::cerr << "Impossible d'ouvrir le fichier profile: " << filename << std::endl;
//        return false;
//    }
//
//    file << j.dump(4);
//    return true;
//}
//
//bool Save::loadProfile(Profile& profile, const std::string& filename)
//{
//    std::ifstream file(filename);
//    if (!file.is_open())
//    {
//        return false;
//    }
//
//    json j;
//    file >> j;
//
//    profile = Profile(j["username"].get<std::string>());
//
//    for (const auto& crewFile : j["crews"])
//    {
//        profile.addCrewFile(crewFile.get<std::string>());
//    }
//
//    return true;
//}

bool Save::saveUser(const Users& user, const std::string& filename)
{
    json j;
    j["name"] = user.getName();
    j["email"] = user.getEmail();
    j["wishes"] = user.getWishes();

    std::ofstream file(filename);
    if (!file.is_open())
    {
        return false;
    }
    file << j.dump(4);
    return true;
}

bool Save::loadUser(Users& user, const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        return false;
    }

    json j;
    file >> j;

    user = Users(j["name"], j["email"]);

    for (const auto& wish : j["wishes"])
        user.addWish(wish);

    return true;
}

bool Save::saveCrew(const Crew& crew, const std::string& filename)
{
    json j;
    j["name"] = crew.getName();
    j["users"] = json::array();

    for (const auto& user : crew.getUsers())
    {
        json ju;
        ju["name"] = user.getName();
        ju["email"] = user.getEmail();
        ju["wishes"] = user.getWishes();
        j["users"].push_back(ju);
    }

    std::ofstream file(filename);
    if (!file.is_open())
    {
        return false;
    }

    file << j.dump(4);
    return true;
}

bool Save::loadCrew(Crew& crew, const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
        return false;

    json j;
    file >> j;

    crew = Crew(j["name"]);

    for (const auto& ju : j["users"])
    {
        Users user(ju["name"], ju["email"]);
        for (const auto& wish : ju["wishes"])
            user.addWish(wish);

        crew.addUser(user);
    }
    return true;
}

bool Save::saveDrawResult(
    const std::string& crewFile,
    const std::map<std::string, std::string>& results)
{
    json j;
    std::ifstream in(crewFile);
    if (!in.is_open())
        return false;

    in >> j;
    in.close();

    j["draw"] = results;

    std::ofstream out(crewFile);
    out << j.dump(4);
    return true;
}

bool Save::loadDrawResult(
    const std::string& crewFile,
    std::map<std::string, std::string>& results)
{
    results.clear();

    std::ifstream in(crewFile);
    if (!in.is_open())
        return false;

    json j;
    in >> j;

    if (!j.contains("draw"))
        return false;

    for (auto it = j["draw"].begin(); it != j["draw"].end(); ++it)
    {
        std::string giver = it.key();
        std::string receiver = it.value().get<std::string>();

        results[giver] = receiver;
    }


    return true;
}



//bool Save::profileExists(const std::string& filename)
//{
//    std::ifstream file(filename);
//    return file.good();
//}

