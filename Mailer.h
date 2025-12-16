#pragma once
#include <map>
#include <string>
#include "Crew.h"

class Mailer
{
    public:
        bool send(const Crew& crew,const std::map<std::string, std::string>& drawResults);
};

