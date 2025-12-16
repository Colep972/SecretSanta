#include "Profile.h"

Profile::Profile()
{

}

Profile::Profile(const std::string& username):m_username(username)
{

}

const std::string& Profile::getUsername() const
{
	return m_username;
}

void Profile::addCrewFile(const std::string& crewFile)
{
	m_crewFiles.push_back(crewFile);
}

const std::vector<std::string>& Profile::getCrewFiles() const
{
	return m_crewFiles;
}
