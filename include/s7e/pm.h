#ifndef S7E_PM_H
#define S7E_PM_H

#include <apr_pools.h>
#include <apr_poll.h>

#include "s7e.h"
#include "s7e/pipe.h"

struct s7e {
    apr_pool_t* pool;

    // configuration
    const char* fs_path;
    s7e_pre_spawn_hook_t* pre_spawn;

    // manager process information
    s7e_proc_status_t pm_status;
    apr_proc_t pm_proc;

    // command pipe
    pipe_t* cmd_pipe;
};

typedef struct {
    s7e_t* shared;
    apr_pollset_t* pollset;
    apr_pool_t* handler_pool;
} s7e_pm_t;

typedef apr_status_t (pm_pollset_handler)(s7e_pm_t*, const apr_pollfd_t*);

apr_status_t pm_main(s7e_t*);
apr_status_t pm_setup_signals(s7e_pm_t*);

#endif /* S7E_PM_H */
