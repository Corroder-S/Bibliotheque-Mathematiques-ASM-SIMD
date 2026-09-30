#include "Menu.h"
#include <iostream>
#include <random>
int main()
{
    std::random_device rd;
    unsigned int seed = rd();
    std::cout << "Seed: " << seed << "\n";
    BenchmarkMenu menu(seed);
    menu.MainMenu();
    return 0;
}
