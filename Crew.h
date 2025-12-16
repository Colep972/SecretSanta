#pragma once
#include <vector>
#include <string>
#include "Users.h"


class Crew
{
    public:
        Crew();
        Crew(const std::string& name);
        void addUser(const Users& user);
        const std::vector<Users>& getUsers() const;
        const std::string& getName() const;
    private:
        std::string m_name;
        std::vector<Users> m_users;
};