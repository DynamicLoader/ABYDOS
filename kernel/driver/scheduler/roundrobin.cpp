#include <map>

#include "k_sysdev.h"

class RoundRobinScheduler : public SysScheduler
{
  public:
    int probe(const char *name, const char *compatible) override
    {
        if (strcmp(name, "scheduler-rr") == 0 && strcmp(compatible, "sys,scheduler") == 0)
            return DRV_CAP_THIS;
        return DRV_CAP_NONE;
    }

    // node should be the hart id
    long addDevice(const void *fdt, int node) override
    {
        (void)fdt;
        _taskgroups[_hdl_count] = taskgroup_t();
        return _hdl_count++;
    }

    void removeDevice(long handler) override
    {
        _taskgroups.erase(handler);
    }

    uint32_t addTask(long hdl, const task_t &task) override
    {
        auto tg = _taskgroups.find(hdl);
        if(tg == _taskgroups.end())
            return 0;
        if(tg->second.tasks.size() == (uint32_t)-1) // already full
            return 0;
        while( tg->second.tasks.find(tg->second.next_task_id) != tg->second.tasks.end()){
            tg->second.next_task_id++; // until found an empty slot
        }
        tg->second.tasks[tg->second.next_task_id] = task;
        return tg->second.next_task_id++;
    }

    int removeTask(long hdl, uint32_t task_id) override
    {
        auto tg = _taskgroups.find(hdl);
        if(tg == _taskgroups.end())
            return K_EINVAL;
        auto task = tg->second.tasks.find(task_id);
        if(task == tg->second.tasks.end())
            return K_EALREADY;
        tg->second.tasks.erase(task);
        return 0;
    }

  private:
    long _hdl_count = 0;
    struct taskgroup_t
    {
        uint32_t next_task_id = 1;
        std::map<uint32_t, task_t> tasks;
    };

    std::map<long, taskgroup_t> _taskgroups;
};

DRV_INSTALL_FUNC(K_PR_DEV_SYSSCHED_END) static void drv_install()
{
    static RoundRobinScheduler scheduler;
    DriverManager::addDriver(scheduler);
    printf("Scheduler RoundRobin installed\n");
}