
#include <cstdint>
#include <cpio.h>


#include "k_vfs.h"
#include "k_defs.h"

class DEVFS : public BasicFS
{
  public:
    DEVFS() {

    }
    ~DEVFS(){

    }

    int open(const char *path, int flags, int mode) override
    {
        return -1;
    }
    int close(int fd) override
    {
        return -1;
    }
    int read(int fd, void *buf, size_t count) override
    {
        return -1;
    }
    int write(int fd, const void *buf, size_t count) override
    {
        return -1;
    }
    int lseek(int fd, off_t offset, int whence) override
    {
        return -1;
    }
    int fstat(int fd, struct stat *buf) override
    {
        return -1;
    }

  private:

};

// Register the filesystem
FS_INSTALL_FUNC(K_PR_FS_BEGIN) static void fs_register()
{
    VirtualFS::registerFS(
        "cpio", [](const char *devicePath) -> std::pair<int, BasicFS *> { return {0, new DEVFS()}; },
        [](BasicFS *fs) -> int {
            delete fs;
            return 0;
        });
    printf("FS DevFS installed\n");
}