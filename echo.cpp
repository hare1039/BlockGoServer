#include <iostream>

int main()
{
	std::string s, all;
	while (std::getline(std::cin, s))
	{
		all += s;
		std::cout << all << std::endl;
	}
}
