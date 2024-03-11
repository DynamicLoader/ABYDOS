
#include <iostream>
#include <string>


#include "k_main.h"
#include "k_sbif.hpp"
#include "k_drvif.h"



SBIF::DebugCon dbg;

std::string getInput(bool echo = true)
{
    char tmp[64] = {0};
    std::string uinput;
    long ret = 0;
    while (true)
    {
        ret = dbg.gets(tmp, 64);
        if (ret == 0)
            continue;

        if (ret > 0)
        {
            for (int i = 0; i < ret; ++i)
            {
                uinput += tmp[i];
                if (echo)
                {
                    std::cout << tmp[i];
                    std::cout.flush();
                }
                if (tmp[i] == '\n' || tmp[i] == '\r')
                    return uinput;
            }
        }
        else
        {
            std::cout << "Error: " << SBIF::getErrorStr(ret) << std::endl;
            return "";
        }
    }
}

int k_main(int argc, const char *argv[])
{
    std::cout << "cmd args: ";
    for (int i = 0; i < argc; ++i)
        std::cout << "'" << argv[i] << "' ";
    std::cout << std::endl;

    // Probing devices
    DriverManager::probe(k_fdt);

    // std::cout << "Please input something: " << std::flush;

    // auto uinput = getInput();
    // std::cout << std::endl << "U input: " << uinput << std::endl;

    // auto ret = SBIF::SRST::reset(SBIF::SRST::WarmReboot, SBIF::SRST::NoReason);
    // std::cout << "Reset: " << SBIF::getErrorStr(ret) << std::endl;
    return 0;
}
