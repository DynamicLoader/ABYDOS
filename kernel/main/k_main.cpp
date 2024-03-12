
#include <iostream>
#include <string>
#include <functional>

#include "k_main.h"
#include "k_sbif.hpp"
#include "k_drvif.h"
#include "k_sysdev.h"

SBIF::DebugCon dbg;

std::function<int(const char *, int size)> k_stdout_func;
bool k_stdout_switched = false;

int k_main(int argc, const char *argv[])
{
    std::cout << "cmd args: ";
    for (int i = 0; i < argc; ++i)
        std::cout << "'" << argv[i] << "' ";
    std::cout << std::endl;

    // Probing system devices
    std::cout << "> Probing system devices" << std::endl;
    DriverManager::probe(k_fdt, DEV_TYPE_SYS);

    SysRoot *sysroot;
    auto srhdl = DriverManager::getDrvByPath(k_fdt, "/", (void **)&sysroot);
    if (srhdl < 0)
    {
        std::cout << "[E] SysRoot not installed properly... Kernel Panic!" << std::endl;
        return K_ENOSPC;
    }
    else
    {
        std::cout << "====================" << std::endl;
        std::cout << "Model: " << sysroot->model() << std::endl;
        std::cout << "Compatible: " << sysroot->compatible() << std::endl;
        std::cout << "Stdout: " << sysroot->stdout_path() << std::endl;
        std::cout << "====================" << std::endl;
    }

    // Probing peripheral devices
    std::cout << "> Probing peripheral devices" << std::endl;
    DriverManager::probe(k_fdt, DEV_TYPE_PERIP);

    if (sysroot->stdout_path().empty())
    {
        std::cout << "[W] Stdout not specified!" << std::endl;
    }
    else
    {
        DriverChar *uart;
        auto stdouthdl = DriverManager::getDrvByPath(k_fdt, sysroot->stdout_path().c_str(), (void **)&uart);
        if (stdouthdl < 0)
        {
            std::cout << "[W] Stdout specified but not installed properly!" << std::endl;
        }
        else
        {
            std::cout << "Switching stdout to " << sysroot->stdout_path() << std::endl;
            k_stdout_func = [stdouthdl, uart](const char *buf, int size) -> int {
                return uart->write(stdouthdl, buf, size);
            };
            k_stdout_switched = true;
            std::cout << "Hello from local driver!" << std::endl;
        }
    }

    // auto ret = SBIF::SRST::reset(SBIF::SRST::WarmReboot, SBIF::SRST::NoReason);
    // std::cout << "Reset: " << SBIF::getErrorStr(ret) << std::endl;
    return 0;
}

// Hook with libc
extern "C" int _write(int fd, char *buf, int size)
{
    if (k_stdout_switched)
        return k_stdout_func(buf, size);
    sbi_ecall(SBI_EXT_DBCN, SBI_EXT_DBCN_CONSOLE_WRITE, size, (unsigned long)buf, 0, 0, 0, 0);
    return size;
}