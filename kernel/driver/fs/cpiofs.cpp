
#include <cstdint>
#include <cstring>
#include <stdexcept>

#include "libcpio/libcpio.h"
#include "k_vfs.h"
#include "k_defs.h"

class CPIOFS : public BasicFS
{
  public:
    CPIOFS(void *archive, unsigned long len) : _archive_len(len)
    {
        printf("%p, %lu\n", archive, len);
        if (cpio_info(archive, len, &_info))
        {
            throw std::runtime_error("cpio_info failed");
        }
        else
        {
            printf("[CPIOFS] cpio_info: %d files, %d max_path_size\n", _info.file_count, _info.max_path_sz);
        }

        _archive = new uint8_t[len];
        memcpy(_archive, archive, len);
    }
    ~CPIOFS()
    {
        if (_archive)
            delete[] _archive;
        _archive = nullptr;
    }

    int open(const char *path, int flags, int mode) override
    {
        unsigned long size = 0;
        const void *file = cpio_get_file(_archive, _archive_len, path, &size);
        if (!file)
        {
            return K_ENOENT;
        }
        for (int i = 0; i < MAX_FILES; i++)
        {
            if (!_fcb[i].used)
            {
                _fcb[i].used = true;
                _fcb[i].file = (void *)file;
                _fcb[i].size = size;
                _fcb[i].lpos = 0;
                return i;
            }
        }
        return K_ENOMEM;
    }
    int close(int fd) override
    {
        if (fd >= MAX_FILES || fd < 0)
            return K_ENOENT;
        if (!_fcb[fd].used)
            return K_ENOENT;
        _fcb[fd].used = false;
        _fcb[fd].file = nullptr;
        _fcb[fd].size = 0;
        _fcb[fd].lpos = 0;
        return K_OK;
    }

    int read(int fd, void *buf, size_t count) override
    {
        if (fd >= MAX_FILES || fd < 0)
            return K_ENOENT;
        if (!_fcb[fd].used)
            return K_ENOENT;
        if (_fcb[fd].lpos + count > _fcb[fd].size)
            count = _fcb[fd].size - _fcb[fd].lpos;
        memcpy(buf, (uint8_t *)_fcb[fd].file + _fcb[fd].lpos, count);
        _fcb[fd].lpos += count;
        return count;
    }

    int write(int fd, const void *buf, size_t count) override
    {
        return -1; // No write should be made to a CPIO archive
    }
    int lseek(int fd, off_t offset, int whence) override
    {
        if (fd >= MAX_FILES || fd < 0)
            return K_ENOENT;
        if (!_fcb[fd].used)
            return K_ENOENT;
        if (whence == SEEK_SET)
        {
            if (offset < 0)
                return K_EINVAL;
            if (offset > (long long)_fcb[fd].size)
                return K_EINVAL;
            _fcb[fd].lpos = offset;
        }
        else if (whence == SEEK_CUR)
        {
            if (_fcb[fd].lpos + offset < 0)
                return K_EINVAL;
            if (_fcb[fd].lpos + offset > _fcb[fd].size)
                return K_EINVAL;
            _fcb[fd].lpos += offset;
        }
        else if (whence == SEEK_END)
        {
            if (offset > 0)
                return K_EINVAL;
            if (offset < -(long long)_fcb[fd].size)
                return K_EINVAL;
            _fcb[fd].lpos = _fcb[fd].size + offset;
        }
        else
        {
            return K_EINVAL;
        }
        return K_OK;
    }
    int fstat(int fd, struct stat *buf) override
    {
        return -1;
    }

  private:
    static constexpr int MAX_FILES = 1024;

    uint8_t *_archive = NULL;
    size_t _archive_len = 0;
    struct cpio_info _info;

    struct fcb_t
    {
        bool used = false;
        void *file = nullptr;
        unsigned long size = 0;
        unsigned long lpos = 0;
    };

    std::array<fcb_t, MAX_FILES> _fcb;
};

// Register the filesystem
FS_INSTALL_FUNC(K_PR_FS_BEGIN) static void fs_register()
{
    VirtualFS::registerFS(
        "cpiofs",
        [](const char *devicePath) -> std::pair<int, BasicFS *> {
            try
            {
                size_t addr = 0, len = 0;
                if (sscanf(devicePath, "%lu,%lu", &addr, &len) != 2 && sscanf(devicePath, "%lx,%lx", &addr, &len) != 2)
                {
                    throw std::runtime_error("invalid devicePath specified");
                }
                auto ret = new CPIOFS((void *)addr, len);
                return {0, ret};
            }
            catch (std::exception &e)
            {
                printf("[CPIO] Failed to create CPIOFS: %s\n", e.what());
                return {-1, nullptr};
            }
            catch (...)
            {
                printf("[CPIO] Failed to create CPIOFS: unknown exception\n");
                return {-2, nullptr};
            }
        },
        [](BasicFS *fs) -> int {
            delete fs;
            return 0;
        });
    printf("FS CPIO installed\n");
}