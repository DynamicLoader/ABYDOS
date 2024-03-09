
#include "k_main.h"
#include "k_sbif.hpp"

// #include <containers>

#include <cstdio>
#include <iostream>

SBIF::DebugCon dbg;

class A
{
  public:
    A()
    {
        dbg.puts("Hello from A!\n");
    }

    ~A()
    {
        dbg.puts("Bye!");
    }

    char *test()
    {
        std::cout << "Going to throw\n";

        char *testp = new char[1048576];
        if (!testp)
            printf("Cannot malloc!");
        testp[0] = 'a';
        throw(int(1));
        return testp;
    }
};

A a;

int k_main(int argc, const char *argv[])
{
    try
    {
        dbg.puts(a.test());
    }
    catch (...)
    {
        printf("Catched\n");
    }

    dbg.puts("cmd args: ");
    for (int i = 0; i < argc; ++i)
    {
        dbg.puts("'");
        dbg.puts(argv[i]);
        dbg.puts("' ");
    }

    return 0;
}
