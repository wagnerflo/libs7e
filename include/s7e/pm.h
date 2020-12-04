#ifndef S7E_PM_H
#define S7E_PM_H

#include <stdbool.h>
#include "s7e/pipe.h"

struct s7e {
    apr_pool_t* pool;

    // configuration
    bool fast_status;
    s7e_pre_spawn_hook_t* pre_spawn;

    // manager process information
    s7e_proc_status_t pm_status;
    apr_proc_t pm_proc;

    // command pipe
    pipe_t* cmd_pipe;
};

void pm_maintenance(int, void*, int);
apr_status_t pm_main(s7e_t*);

#endif /* S7E_PM_H */
