#include "Users.h"

Users::Users(const std::string& name, const std::string& email):m_name(name)
,m_email(email)
{

}

const std::string& Users::getName() const
{
	return m_name;
}

const std::string& Users::getEmail() const
{
	return m_email;
}

void Users::setName(std::string name)
{
	m_name = name;
}

void Users::setMail(std::string email)
{
	m_email = email;
}

void Users::addWish(const std::string& wish)
{
	m_wishes.push_back(wish);
}

const std::vector<std::string>& Users::getWishes() const
{
	return m_wishes;
}
