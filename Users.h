class Users
{
    public:
    Users();
    Users(const std::string& name, const std::string& email);
    const std::string& getName() const;
    const std::string& getEmail() const;
    void addWish(const std::string& wish);
    const std::vector<std::string>& getWishes() const;
    private:
    std::string m_name;
    std::string m_email;
    std::vector <std::string> m_wishes;
}