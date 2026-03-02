#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <map>

struct UserMail 
{
	std::string name;
	std::string email;
};

class SecretSanta
{
	public:
		SecretSanta();
		~SecretSanta();
		bool init();
		void Pull();
		void Show();
		int getnbParticipants();

	private:
		std::map<std::string, std::string> santaMap;
		std::vector<std::string> participants;
		std::vector<std::string> stock;
		int m_nbParticipants;
		int* interdit;
};

