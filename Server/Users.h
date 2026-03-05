#pragma once
#include <string>
#include <vector>

class Users
{
public:
    Users() = default;
    Users(const std::string& name, const std::string& email);
    const std::string& getName() const;
    const std::string& getEmail() const;
    const std::string& getToken() const;
    void setName(std::string name);
    void setMail(std::string email);
    void setToken(const std::string& token);
    void addWish(const std::string& wish);
    bool removeWish(int index);   // 0-based, returns false if out of range
    void clearWishes();
    const std::vector<std::string>& getWishes() const;
private:
    std::string m_name;
    std::string m_email;
    std::string m_token;
    std::vector<std::string> m_wishes;
};