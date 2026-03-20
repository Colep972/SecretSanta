bool Save::savePots(const std::vector<Pot>& pots, const std::string& filename)
{
    json j = json::array();
    for (const auto& pot : pots)
    {
        json jp;
        jp["id"] = pot.getId();
        jp["name"] = pot.getName();
        jp["amount"] = pot.getAmount();
        jp["creator"] = pot.getCreator();
        jp["participants"] = json::array();
        for (const auto& p : pot.getParticipants())
        {
            json jpp;
            jpp["name"] = p.name;
            jpp["paid"] = p.paid;
            jp["participants"].push_back(jpp);
        }
        j.push_back(jp);
    }
    std::ofstream file(filename);
    if (!file.is_open()) return false;
    file << j.dump(4);
    return true;
}

bool Save::loadPots(std::vector<Pot>& pots, const std::string& filename)
{
    pots.clear();
    std::ifstream file(filename);
    if (!file.is_open()) return false;
    json j; file >> j;
    for (const auto& jp : j)
    {
        Pot pot(
            jp["id"].get<std::string>(),
            jp["name"].get<std::string>(),
            jp["amount"].get<double>(),
            jp["creator"].get<std::string>()
        );
        for (const auto& jpp : jp["participants"])
        {
            pot.addParticipant(jpp["name"].get<std::string>());
            if (jpp["paid"].get<bool>())
                pot.markPaid(jpp["name"].get<std::string>());
        }
        pots.push_back(pot);
    }
    return true;
}