#pragma once
#include <string>
#include "Profile.h"
#include "Users.h"
#include "Crew.h"

class Save
{
public:
    static bool saveProfile(const Profile& profile, const std::string& filename);
    static bool loadProfile(Profile& profile, const std::string& filename);
    static bool profileExists(const std::string& filename);

    static bool saveUser(const Users& user, const std::string& filename);
    static bool loadUser(Users& user, const std::string& filename);

    static bool saveCrew(const Crew& crew, const std::string& filename);
    static bool loadCrew(Crew& crew, const std::string& filename);


};
