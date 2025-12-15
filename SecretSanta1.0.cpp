#include "SecretSanta.h"

int main()
{
	SecretSanta Ss;
	if (Ss.init())
	{
		Ss.Pull();
		Ss.SendEmails();
	}
	return 0;
}