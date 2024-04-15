#include <Windows.h>

#include <fstream>
#include <random>

#include "CascadeSort.h"

class RNGint
{
public:
	static int Get(int leftBound, int righBound)
	{
		std::uniform_int_distribution<int> dist{ leftBound, righBound };
		return dist(s_rng);
	}

private:
	static std::mt19937 s_rng;
};
std::mt19937 RNGint::s_rng{ static_cast<unsigned int>(std::time(nullptr)) };

int main()
{
	SetConsoleOutputCP(65001);

	std::ofstream file{ "input.txt", std::ios::trunc };
	for (int i = 1; i <= 100; ++i)
	{
		file << RNGint::Get(0, 300) << ' ';
	}
	file.close();

	CascadeSort("input.txt", 6);
	return 0;
}