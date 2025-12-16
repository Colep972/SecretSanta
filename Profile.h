#pragma once
#include <string>
#include <vector>

class Profile
{
    public:
        Profile();
        Profile(const std::string& username);

        const std::string& getUsername() const;

        void addCrewFile(const std::string& crewFile);
        const std::vector<std::string>& getCrews() const;
        const std::vector<std::string>& getCrewFiles() const;

    private:
        std::string m_username;
        std::vector<std::string> m_crewFiles;
};

