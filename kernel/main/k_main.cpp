
#include "k_main.h"
#include "k_sbif.hpp"



// #include <containers>

// class A{
// public:
//     ~A(){
//         SBIF::DebugCon dbg;
//         dbg.puts("Bye!");
//     }   

// void test(){}

// };

int k_main(int argc, const char *argv[])
{
    SBIF::DebugCon dbg;
    // A a;
    // a.test();
    dbg.puts("cmd args: ");
    for (int i = 0; i < argc; ++i)
    {
        dbg.puts("'");
        dbg.puts(argv[i]);
        dbg.puts("' ");
    }

    dbg.puts("\nTrying to shutdown...\n");
    if (SBIF::SRST::reset(SBIF::SRST::Shutdown, SBIF::SRST::NoReason))
    {
        dbg.puts("Cannot shutdown!\n");
    }
    return 0;
}
