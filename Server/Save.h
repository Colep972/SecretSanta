#pragma once
#include <string>
//#include "../Profile.h"
#include "Users.h"
#include "Crew.h"
#include <map>

class Save
{
public:
    /*static bool saveProfile(const Profile& profile, const std::string& filename);
    static bool loadProfile(Profile& profile, const std::string& filename);
    static bool profileExists(const std::string& filename);*/

    static bool saveUser(const Users& user, const std::string& filename);
    static bool loadUser(Users& user, const std::string& filename);

    static bool saveCrew(const Crew& crew, const std::string& filename);
    static bool loadCrew(Crew& crew, const std::string& filename);

    static bool saveDrawResult(const std::string& crewFile,
        const std::map<std::string, std::string>& results);
    static bool loadDrawResult(
        const std::string& crewFile,
        std::map<std::string, std::string>& results);




};
