#pragma once
#include <string>
#include <vector>

struct PotParticipant
{
    std::string name;
    bool paid = false;
};

class Pot
{
public:
    Pot() = default;
    Pot(const std::string& id, const std::string& name,
        double amount, const std::string& creator);

    const std::string& getId()      const;
    const std::string& getName()    const;
    double             getAmount()  const;
    const std::string& getCreator() const;
    const std::vector<PotParticipant>& getParticipants() const;

    void addParticipant(const std::string& name);
    bool markPaid(const std::string& name);
    bool hasMember(const std::string& name) const;
    bool hasPaid(const std::string& name)   const;

private:
    std::string m_id;
    std::string m_name;
    double      m_amount;
    std::string m_creator;
    std::vector<PotParticipant> m_participants;
};