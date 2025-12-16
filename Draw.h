#include <map>
#include "Crew.h"
#include <string>

class Draw
{
    public:
        Draw();
        bool run(const Crew& crew);
        const std::map<std::string, std::string>& getResults() const;
    private:
        std::map<std::string, std::string> m_results;
};