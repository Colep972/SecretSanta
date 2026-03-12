#pragma once
#include <string>
#include <vector>

struct CrewEntry
{
    std::string code;
    std::string token;
};

class Profile
{
public:
    Profile() = default;
    Profile(const std::string& username, const std::string& passwordHash,
        const std::string& name, const std::string& email);

    const std::string& getUsername()     const;
    const std::string& getPasswordHash() const;
    const std::string& getName()         const;
    const std::string& getEmail()        const;
    const std::string& getToken()        const;
    const std::vector<std::string>& getWishes() const;
    const std::vector<CrewEntry>& getCrews()  const;

    void setName(const std::string& n);
    void setEmail(const std::string& e);
    void setToken(const std::string& t);

    void addWish(const std::string& w);
    void removeWish(int index);
    void clearWishes();

    void addCrew(const std::string& code, const std::string& token);
    const std::string* getCrewToken(const std::string& code) const;

private:
    std::string m_username;
    std::string m_passwordHash;
    std::string m_name;
    std::string m_email;
    std::string m_token;
    std::vector<std::string> m_wishes;
    std::vector<CrewEntry>   m_crews;
};