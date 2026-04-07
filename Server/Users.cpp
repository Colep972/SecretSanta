#include "Users.h"
#include <iostream>

Users::Users(const std::string& name, const std::string& email)
    : m_name(name), m_email(email)
{
}

const std::string& Users::getName() const { return m_name; }
const std::string& Users::getEmail() const { return m_email; }
const std::string& Users::getToken() const { return m_token; }

void Users::setName(std::string name) { m_name = name; }
void Users::setMail(std::string email) { m_email = email; }
void Users::setToken(const std::string& token) { m_token = token; }

void Users::addWish(const std::string& wish)
{
    m_wishes.push_back(wish);
}

bool Users::removeWish(int index)
{
    if (index < 0 || index >= (int)m_wishes.size())
    {
        std::cout << "Il n'y a pas de voeux correspondant" << std::endl;
        return false;
    }
    m_wishes.erase(m_wishes.begin() + index);
    return true;
}

void Users::clearWishes()
{
    m_wishes.clear();
}

const std::vector<std::string>& Users::getWishes() const
{
    return m_wishes;
}