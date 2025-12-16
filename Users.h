#pragma once
#include <string>
#include <vector>

class Users
{
    public:
        Users(const std::string& name, const std::string& email);
        const std::string& getName() const;
        const std::string& getEmail() const;
        void setName(std::string name);
        void setMail(std::string email);
        void addWish(const std::string& wish);
        const std::vector<std::string>& getWishes() const;
    private:
        std::string m_name;
        std::string m_email;
        std::vector <std::string> m_wishes;
};
