#ifndef S7E_PM_H
#define S7E_PM_H

#include <apr_hash.h>
#include <apr_poll.h>
#include <apr_pools.h>

#include "s7e.h"
#include "s7e/pipe.h"
#include "s7e/bitset.h"

struct s7e {
    apr_pool_t* pool;

    // configuration
    unsigned int max_proc;
    const char* fs_path;
    s7e_pre_spawn_hook_t* pre_spawn;

    // manager process information
    s7e_proc_status_t pm_status;
    apr_proc_t pm_proc;

    // command pipe
    pipe_t* cmd_pipe;
};

typedef struct {
    // s7e struct inherited from the process managers parent
    s7e_t* shared;

    // pollset and memory pool for the main event loop
    apr_pollset_t* pollset;
    apr_pool_t* handler_pool;

    //
    bitset_t proc_map;
    apr_hash_t* proc_config;
    uint32_t* proc_status;
} pm_t;

typedef apr_status_t (pm_pollset_handler)(pm_t*, const apr_pollfd_t*);

apr_status_t pm_main(s7e_t*);
apr_status_t pm_setup_signals(pm_t*);
apr_status_t pm_setup_proctbl(pm_t*);
apr_status_t pm_setup_cmd(pm_t*);

#endif /* S7E_PM_H */
