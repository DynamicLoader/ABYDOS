#include "k_main.h"
#include "k_vfs.h"

/* __attribute__((
    init_priority(K_PR_INIT_FS_LIST)))  */
std::map<std::string, std::pair<BasicFS *, VirtualFS::deleteInstanceFunc_t>> VirtualFS::_fs_map;

__attribute__((init_priority(K_PR_INIT_FS_LIST)))
std::vector<std::tuple<std::string, VirtualFS::newInstanceFunc_t, VirtualFS::deleteInstanceFunc_t>>
    VirtualFS::_fs_factories;

__attribute__((init_priority(K_PR_INIT_FS_LIST))) lock_t VirtualFS::_fs_lock;

std::map<int, std::pair<BasicFS *, int>> VirtualFS::_global_fd_map;
int VirtualFS::_global_fd_counter = 3; // 0, 1, 2 are reserved for stdin, stdout, stderr of system

int VirtualFS::_write_stdout(const char *buf, int size)
{
    static int support_dbcn = -1;
    if (support_dbcn < 0)
    {
        support_dbcn = sbi_ecall(SBI_EXT_BASE, SBI_EXT_BASE_PROBE_EXT, SBI_EXT_DBCN, 0, 0, 0, 0, 0).value;
    }
    if (k_stdout_switched)
        return k_stdout_func(buf, size);
    if (support_dbcn > 0)
        sbi_ecall(SBI_EXT_DBCN, SBI_EXT_DBCN_CONSOLE_WRITE, size, (unsigned long)buf & 0xffffffff, 0, 0, 0, 0);
    else
    {
        for (int i = 0; i < size; i++)
        {
            sbi_ecall(SBI_EXT_0_1_CONSOLE_PUTCHAR, 0, buf[i], 0, 0, 0, 0, 0);
        }
    }
    return size;
}