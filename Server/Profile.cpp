#include "Profile.h"

Profile::Profile(const std::string& username, const std::string& passwordHash,
    const std::string& name, const std::string& email)
    : m_username(username), m_passwordHash(passwordHash),
    m_name(name), m_email(email) {
}

const std::string& Profile::getUsername()     const { return m_username; }
const std::string& Profile::getPasswordHash() const { return m_passwordHash; }
const std::string& Profile::getName()         const { return m_name; }
const std::string& Profile::getEmail()        const { return m_email; }
const std::string& Profile::getToken()        const { return m_token; }
const std::vector<std::string>& Profile::getWishes() const { return m_wishes; }
const std::vector<CrewEntry>& Profile::getCrews()  const { return m_crews; }

void Profile::setName(const std::string& n) { m_name = n; }
void Profile::setEmail(const std::string& e) { m_email = e; }
void Profile::setToken(const std::string& t) { m_token = t; }

void Profile::addWish(const std::string& w) { m_wishes.push_back(w); }
void Profile::removeWish(int index) { m_wishes.erase(m_wishes.begin() + index); }
void Profile::clearWishes() { m_wishes.clear(); }

void Profile::addCrew(const std::string& code, const std::string& token)
{
    for (auto& c : m_crews)
        if (c.code == code) { c.token = token; return; }
    m_crews.push_back({ code, token });
}

const std::string* Profile::getCrewToken(const std::string& code) const
{
    for (const auto& c : m_crews)
        if (c.code == code) return &c.token;
    return nullptr;
}