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
    std::vector<Users>& getUsers();
    const std::string& getName() const;
    const std::string& getOwnerToken() const;
    void setOwnerToken(const std::string& token);

    // Returns pointer to user, nullptr if not found
    Users* findUserByToken(const std::string& token);
    const Users* findUserByName(const std::string& name) const;
private:
    std::string m_name;
    std::string m_ownerToken;
    std::vector<Users> m_users;
};