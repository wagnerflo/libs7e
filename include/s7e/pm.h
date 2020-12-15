#ifndef S7E_PM_H
#define S7E_PM_H

#include <apr_hash.h>
#include <apr_poll.h>
#include <apr_pools.h>
#include <apr_shm.h>
#include <apr_skiplist.h>
#include <apr_thread_proc.h>

#include "s7e/pipe.h"
#include "s7e/bitset.h"
#include "s7e.h"

#define PM_IS_UP             1
#define PM_IS_CHILD          2
#define PM_IS_PARENT         4
#define PM_IS_REMOTE         8
#define PM_HAS_CMD_PIPE     16
#define PM_HAS_FAST_STATUS  32

#define PARENT_PRE_START(pm) \
  (pm->status & PM_IS_PARENT || !(pm->status & PM_IS_UP))

#define PARENT_POST_START(pm) \
  (pm->status & PM_IS_PARENT && pm->status & PM_IS_UP)

struct s7e {
    int status;
    apr_pool_t* parent_pool;
    apr_pool_t* pool;

    // configuration
    unsigned int max_proc;
    s7e_pre_spawn_hook_t* pre_spawn;

    // fast status
    apr_shm_t* fs_shm;
    s7e_proc_status_t* fs_base;

    // manager process information
    apr_proc_t* pm_proc;

    // command pipe
    pipe_t* cmd_pipe;
};

typedef struct {
    // s7e struct inherited from the process managers parent
    s7e_t* shared;

    // pollset and memory pool for the main event loop
    apr_pollset_t* pollset;
    apr_pool_t* handler_pool;

    // process management
    bitset_t proc_map;
    apr_hash_t* proc_config;
    apr_skiplist* proc_actions;
} pm_t;

typedef apr_status_t (pm_pollset_handler)(pm_t*, const apr_pollfd_t*);
typedef apr_status_t (action_handler)(pm_t*, apr_pool_t*, void*);

typedef struct {
    apr_pool_t* pool;
    unsigned int* key;
    char** argv;
} proc_config_t;

typedef struct {
    apr_time_t when;
    action_handler* handler;
    void* data;
} action_t;

apr_status_t pm_main(s7e_t*);
apr_status_t pm_setup_signals(pm_t*);
apr_status_t pm_setup_proctbl(pm_t*);
apr_status_t pm_setup_actions(pm_t*);
apr_status_t pm_setup_cmd(pm_t*);

apr_status_t proctbl_add(pm_t*, proc_config_t**);
apr_status_t proctbl_remove(pm_t*, unsigned int);

apr_interval_time_t actions_poll_timeout(pm_t*);
apr_status_t actions_handle_current(pm_t*, apr_pool_t*);
apr_status_t actions_add_now(pm_t*, action_handler*, void*);

apr_status_t action_start(pm_t*, apr_pool_t*, void*);

#endif /* S7E_PM_H */
