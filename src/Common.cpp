#include "Common.h"

int GetIntFromString(const std::string & s)
{
	std::stringstream ss(s);
	int a = 0;
	ss >> a;
	return a;
}

bool EqualIgnoreCase(const std::string & s1, const std::string & s2)
{
	return s1.length() == s2.length() && std::equal(s1.begin(), s1.end(), s2.begin(), s2.end(), [](char i, char j)
	{
		return std::tolower(i) == std::tolower(j);
	});
}