class Crew
{
    public:
    Crew(const std::string& name);
    void addUser(const User& user);
    const std::vector<User>& getUsers() const;
    const std::string& getName() const;
    size_t size() const;
    private:
    std::string m_name;
    std::vector<User> m_users;
}