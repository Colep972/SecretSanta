#include "Pot.h"

Pot::Pot(const std::string& id, const std::string& name,
    double amount, const std::string& creator)
    : m_id(id), m_name(name), m_amount(amount), m_creator(creator) {
}

const std::string& Pot::getId()      const { return m_id; }
const std::string& Pot::getName()    const { return m_name; }
double             Pot::getAmount()  const { return m_amount; }
const std::string& Pot::getCreator() const { return m_creator; }
const std::vector<PotParticipant>& Pot::getParticipants() const { return m_participants; }

void Pot::addParticipant(const std::string& name)
{
    m_participants.push_back({ name, false });
}

bool Pot::markPaid(const std::string& name)
{
    for (auto& p : m_participants)
        if (p.name == name) { p.paid = true; return true; }
    return false;
}

bool Pot::hasMember(const std::string& name) const
{
    for (const auto& p : m_participants)
        if (p.name == name) return true;
    return false;
}

bool Pot::hasPaid(const std::string& name) const
{
    for (const auto& p : m_participants)
        if (p.name == name) return p.paid;
    return false;
}