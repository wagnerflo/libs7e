#ifndef S7E_PM_H
#define S7E_PM_H

#include <stdbool.h>

struct s7e {
    apr_pool_t* pool;

    // configuration
    bool fast_status;
    s7e_pre_spawn_hook_t* pre_spawn;

    // manager process information
    s7e_proc_status_t pm_status;
    apr_proc_t pm_proc;
};

void run_maintenance(int, void*, int);
void pm_main(s7e_t*);

#endif /* S7E_PM_H */
