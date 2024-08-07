#ifndef __K_VFS_H__
#define __K_VFS_H__

#include <cstdio>
#include <string>
#include <functional>
#include <sys/lock.h>

// #include <filesystem>
// #include "k_drvif.h"
#include "k_defs.h"
#include "k_lock.h"

#define FS_INSTALL_FUNC(V) __attribute__((constructor(V)))

class BasicFS
{
  public:
    virtual int open(const char *path, int flags, int mode) = 0;
    virtual int close(int fd) = 0;
    virtual int read(int fd, void *buf, size_t count) = 0;
    virtual int write(int fd, const void *buf, size_t count) = 0;
    virtual int lseek(int fd, off_t offset, int whence) = 0;
    virtual int fstat(int fd, struct stat *buf) = 0;
    // virtual int stat(const char *path, struct stat *buf) = 0;

    // virtual int opendir(const char *path) = 0;
    // virtual int readdir(int fd, struct dirent *dirp) = 0;
    // virtual int closedir(int fd) = 0;

    // virtual int mkdir(const char *path, mode_t mode) = 0;
    // virtual int rmdir(const char *path) = 0;

    virtual ~BasicFS() = default;
};

constexpr int FILE_STDIN = 0;
constexpr int FILE_STDOUT = 1;
constexpr int FILE_STDERR = 2;

class VirtualFS
{
  public:
    using newInstanceFunc_t = std::function<std::pair<int, BasicFS *>(const char *devicePath)>;
    using deleteInstanceFunc_t = std::function<int(BasicFS *)>;

    VirtualFS();
    ~VirtualFS();

    static int registerFS(const std::string &fs_name, newInstanceFunc_t nf, deleteInstanceFunc_t df)
    {
        if (!nf || !df)
        {
            return K_EINVAL;
        }
        _fs_factories.push_back(std::make_tuple(fs_name, nf, df));
        return 0;
    }
    static int unregisterFS(const std::string &fs_name)
    {
        for (auto it = _fs_factories.begin(); it != _fs_factories.end(); it++)
        {
            if (std::get<0>(*it) == fs_name)
            {
                _fs_factories.erase(it);
                return 0;
            }
        }
        return K_ENOENT;
    }

    // POSIX-like functionss
    static int open(const char *path, int flags, int mode)
    {
        // printf("Open %s", path);
        _fs_lock.lock();
        BasicFS *fs = nullptr;
        size_t max_len = 0;
        for (auto x : _fs_map)
        {
            if (std::string_view(path).find(x.first) == 0)
            {
                if (max_len < x.first.size())
                {
                    max_len = x.first.size();
                    fs = x.second.first;
                }
            }
        }
        if (fs)
        {
            int ret = fs->open(path+max_len, flags, mode);
            if (ret >= 0)
            {
                auto fd = _global_fd_counter++;
                _global_fd_map.insert(std::make_pair(fd, std::make_pair(fs, ret)));
                _fs_lock.unlock();
                return fd;
            }

            _fs_lock.unlock();
            return ret;
        }
        // printf("Failed!\n");
        _fs_lock.unlock();
        return K_ENOENT;
    }
    static int close(int fd)
    {
        _fs_lock.lock();
        auto it = _global_fd_map.find(fd);
        if (it != _global_fd_map.end())
        {
            int ret = it->second.first->close(it->second.second);
            if (ret == K_OK)
                _global_fd_map.erase(it);
            _fs_lock.unlock();
            return ret;
        }
        _fs_lock.unlock();
        return K_ENOENT;
    }
    static int read(int fd, void *buf, int count)
    {
        if (fd == FILE_STDIN)
        {
            // STDIN
            return 0;
        }

        _fs_lock.lock();
        auto it = _global_fd_map.find(fd);
        _fs_lock.unlock();

        if (it != _global_fd_map.end())
        {
            return it->second.first->read(it->second.second, buf, count);
        }

        return K_ENOENT;
    }

    static int write(int fd, const void *buf, int count)
    {
        if (fd == FILE_STDOUT)
        {
            return _write_stdout((const char *)buf, count);
        }

        _fs_lock.lock();
        auto it = _global_fd_map.find(fd);
        _fs_lock.unlock();

        if (it != _global_fd_map.end())
        {
            return it->second.first->write(it->second.second, buf, count);
        }

        return K_ENOENT;
    }

    static int lseek(int fd, off_t offset, int whence)
    {
        if(fd == FILE_STDIN || fd == FILE_STDOUT || fd == FILE_STDERR)
            return K_ENOTSUPP;

        _fs_lock.lock();
        auto it = _global_fd_map.find(fd);
        _fs_lock.unlock();
        
        if (it != _global_fd_map.end())
        {
            return it->second.first->lseek(it->second.second, offset, whence);
        }

        return K_ENOENT;
    }
    static int fstat(int fd, struct stat *buf)
    {
        return -1;
    }

    static int mount(const char *path, const char *devicePath, int flags, int mode, const char *fs_name = nullptr)
    {
        _fs_lock.lock();
        for (auto &fs : _fs_factories)
        {
            if ((fs_name && std::get<0>(fs) == fs_name) || !fs_name)
            {
                auto [ret, fs_instance] = std::get<1>(fs)(devicePath);
                if (ret == K_OK)
                {
                    _fs_map.insert(std::make_pair(path, std::make_pair(fs_instance, std::get<2>(fs))));
                    _fs_lock.unlock();
                    return 0;
                }
                if (ret != K_ENOTSUPP) // Something other than not supported happened
                {
                    _fs_lock.unlock();
                    return ret;
                }
            }
        }
        _fs_lock.unlock();
        return K_ENOTSUPP; // No FS could be mounted
    }

    static int umount(const char *path) // After unmounting, the FS instance is deleted
    {
        _fs_lock.lock();
        auto it = _fs_map.find(path);
        if (it != _fs_map.end())
        {
            it->second.second(it->second.first);
            _fs_map.erase(it);
            _fs_lock.unlock();
            return 0;
        }
        _fs_lock.unlock();
        return K_ENOTSUPP;
    }

    // int stat(const char *path, struct stat *buf);

    // int opendir(const char *path);
    // int readdir(int fd, struct dirent *dirp);
    // int closedir(int fd);

  private:
    static std::map<std::string, std::pair<BasicFS *, deleteInstanceFunc_t>> _fs_map;
    static std::vector<std::tuple<std::string, newInstanceFunc_t, deleteInstanceFunc_t>>
        _fs_factories; // fs-name, new-instance-func, delete-instance-func
    static lock_t _fs_lock;
    static std::map<int, std::pair<BasicFS *, int>> _global_fd_map;
    static int _global_fd_counter;

    static int _write_stdout(const char *buf, int size);
};

#endif