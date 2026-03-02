#include "SecretSanta.h"

int main()
{
	SecretSanta Ss;
	if (Ss.init())
	{
		Ss.Pull();
		Ss.Show();
	}
	return 0;
}