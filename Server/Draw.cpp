#include "Draw.h"
#include <iostream>

Draw::Draw()
{

}

bool Draw::run(const Crew& crew)
{
    m_results.clear();

    const auto& users = crew.getUsers();
    int nbParticipants = users.size();

    if (nbParticipants < 2)
    {
        return false;
    }

    
    std::vector<std::string> participants;
    for (const auto& user : users)
    {
        participants.push_back(user.getName());
    }

    std::vector<std::string> stock = participants;

    srand(static_cast<unsigned int>(time(nullptr)));

    for (int i = stock.size() - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        std::swap(stock[i], stock[j]);
    }

    bool valid = false;
    while (!valid)
    {
        valid = true;

        for (int i = 0; i < nbParticipants; i++)
        {
            if (stock[i] == participants[i])
            {
                valid = false;
                break;
            }
        }

        if (!valid)
        {
            for (int i = stock.size() - 1; i > 0; i--)
            {
                int j = rand() % (i + 1);
                std::swap(stock[i], stock[j]);
            }
        }
    }

    for (int i = 0; i < nbParticipants; i++)
    {
        m_results[participants[i]] = stock[i];
    }

    return true;
}

const std::map<std::string, std::string>& Draw::getResults() const
{
	return m_results;
}
