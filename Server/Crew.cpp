#include "Crew.h"

Crew::Crew() {}

Crew::Crew(const std::string& name) : m_name(name) {}

void Crew::addUser(const Users& user)
{
    m_users.push_back(user);
}

bool Crew::removeUser(const std::string& name)
{
    for (auto it = m_users.begin(); it != m_users.end(); ++it)
    {
        if (it->getName() == name)
        {
            m_users.erase(it);
            return true;
        }
    }
    return false;
}

const std::vector<Users>& Crew::getUsers() const { return m_users; }
std::vector<Users>& Crew::getUsers() { return m_users; }
const std::string& Crew::getName() const { return m_name; }
const std::string& Crew::getOwnerToken() const { return m_ownerToken; }
void Crew::setOwnerToken(const std::string& token) { m_ownerToken = token; }

Users* Crew::findUserByToken(const std::string& token)
{
    for (auto& u : m_users)
        if (u.getToken() == token && !token.empty())
            return &u;
    return nullptr;
}

const Users* Crew::findUserByName(const std::string& name) const
{
    for (const auto& u : m_users)
        if (u.getName() == name)
            return &u;
    return nullptr;
}

bool Crew::removeParticipant(const std::string& name)
{
    for (auto it = m_users.begin(); it != m_users.end(); ++it)
    {
        if (it->getName() == name)
        {
            m_users.erase(it);
            return true;
        }
    }
    return false;
}