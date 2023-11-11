#include <iostream>

#include "../inc/Config.hpp"

int main(int argc, char* argv[])
{
    Config cfg;
    cfg.readConfigFile("../cfg/cfg.json");
    return 0;
}
