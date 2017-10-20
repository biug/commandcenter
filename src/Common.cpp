#include "Common.h"

int GetIntFromString(const std::string & s)
{
	std::stringstream ss(s);
	int a = 0;
	ss >> a;
	return a;
}