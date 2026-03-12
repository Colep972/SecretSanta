#pragma once
#include <string>
#include <map>
#include "Crew.h"
#include "Users.h"
#include "Profile.h"

class Save
{
public:
    static bool saveUser(const Users& user, const std::string& filename);
    static bool loadUser(Users& user, const std::string& filename);

    static bool saveCrew(const Crew& crew, const std::string& filename);
    static bool loadCrew(Crew& crew, const std::string& filename);

    static bool saveDrawResult(const std::string& crewFile,
        const std::map<std::string, std::string>& results);
    static bool loadDrawResult(const std::string& crewFile,
        std::map<std::string, std::string>& results);
    static bool clearDrawResult(const std::string& crewFile);

    static bool saveProfile(const Profile& profile, const std::string& filename);
    static bool loadProfile(Profile& profile, const std::string& filename);
};