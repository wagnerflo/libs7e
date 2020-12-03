#ifndef S7E_H
#define S7E_H

#include <apr_thread_proc.h>

struct s7e;

struct s7e_process;

struct s7e_event {
    unsigned int type;
    pid_t pid;
};

typedef int s7e_proc_status_t;
typedef struct s7e s7e_t;
typedef apr_status_t (s7e_pre_spawn_hook_t)(apr_pool_t*, s7e_t*);

#define S7E_PROC_IS_RUNNING(status) (status > 0)
#define S7E_PROC_UP       1
#define S7E_PROC_DOWN     0
#define S7E_PROC_STOPPED -1

#define S7E_START_ERROR (APR_OS_START_USERERR + 15000)
#define S7E_ALREADY_STARTED (S7E_START_ERROR + 1)

s7e_t* s7e_init(apr_pool_t*);
apr_status_t s7e_set_prespawn_hook(s7e_t*, s7e_pre_spawn_hook_t*);
apr_status_t s7e_start(s7e_t*);

#endif /* S7E_H */
