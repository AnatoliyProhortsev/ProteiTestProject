#include <iostream>

#include "../inc/CallCenter.hpp"

int main(int argc, char* argv[])
{
    CallCenter center("../cfg/cfg.json");
    center.start();
    center.exportCDR();
    return 0;
}
