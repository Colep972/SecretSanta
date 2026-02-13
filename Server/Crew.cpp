#include "Crew.h"

Crew::Crew()
{

}

Crew::Crew(const std::string& name):m_name(name)
{

}

void Crew::addUser(const Users& user)
{
	m_users.push_back(user);
}

const std::vector<Users>& Crew::getUsers() const
{
	return m_users;
}

const std::string& Crew::getName() const
{
	return m_name;
}
