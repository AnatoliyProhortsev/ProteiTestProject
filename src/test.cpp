#include <iostream>

#include "../inc/CallCenter.hpp"

int main(int argc, char* argv[])
{
    CallCenter center("../cfg/cfg.json");
    center.start();
    std::cout<<center.exportCDR()<<'\n';
    return 0;
}
