#include "Save.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

bool Save::saveUser(const Users& user, const std::string& filename)
{
    json j;
    j["name"] = user.getName();
    j["email"] = user.getEmail();
    j["token"] = user.getToken();
    j["wishes"] = user.getWishes();

    std::ofstream file(filename);
    if (!file.is_open())
        return false;

    file << j.dump(4);
    return true;
}

bool Save::loadUser(Users& user, const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
        return false;

    json j;
    file >> j;

    user = Users(j["name"], j["email"]);

    if (j.contains("token"))
        user.setToken(j["token"].get<std::string>());

    for (const auto& wish : j["wishes"])
        user.addWish(wish);

    return true;
}

bool Save::saveCrew(const Crew& crew, const std::string& filename)
{
    json j;
    j["name"] = crew.getName();
    j["owner_token"] = crew.getOwnerToken();
    j["users"] = json::array();

    for (const auto& user : crew.getUsers())
    {
        json ju;
        ju["name"] = user.getName();
        ju["email"] = user.getEmail();
        ju["token"] = user.getToken();
        ju["wishes"] = user.getWishes();
        j["users"].push_back(ju);
    }

    std::ofstream file(filename);
    if (!file.is_open())
        return false;

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

    if (j.contains("owner_token"))
        crew.setOwnerToken(j["owner_token"].get<std::string>());

    for (const auto& ju : j["users"])
    {
        Users user(ju["name"], ju["email"]);
        if (ju.contains("token"))
            user.setToken(ju["token"].get<std::string>());
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
    std::ifstream in(crewFile);
    if (!in.is_open())
        return false;

    json j;
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
        results[it.key()] = it.value().get<std::string>();

    return true;
}

bool Save::clearDrawResult(const std::string& crewFile)
{
    std::ifstream in(crewFile);
    if (!in.is_open())
        return false;

    json j;
    in >> j;
    in.close();

    j.erase("draw");

    std::ofstream out(crewFile);
    if (!out.is_open())
        return false;

    out << j.dump(4);
    return true;
}