#include <stdio.h>
#include <apr_strings.h>

#include <stdlib.h>
#include <apr_general.h>
#include <apr_pools.h>
#include <apr_thread_proc.h>

#include "s7e.h"
#include "s7e/pm.h"
#include "s7e/pipe.h"
#include "s7e/proto.h"
#include "cmd.pb-c.h"

s7e_t* s7e_init(apr_pool_t* pool) {
  s7e_t* pm = apr_pcalloc(pool, sizeof(s7e_t));
  pm->pool = pool;
  return pm;
}

apr_status_t s7e_set_prespawn_hook(s7e_t* pm, s7e_pre_spawn_hook_t* hook) {
  if (pm->pm_status != S7E_PROC_DOWN)
    return S7E_ALREADY_STARTED;

  pm->pre_spawn = hook;
}

apr_status_t s7e_enable_fast_status(s7e_t* pm) {
  if (pm->pm_status != S7E_PROC_DOWN)
    return S7E_ALREADY_STARTED;

  /* TODO: check if OS supports shm and atomic_store/_load */

  pm->fast_status = 1;
}

apr_status_t s7e_start(s7e_t* pm) {
  if (S7E_PROC_IS_RUNNING(pm->pm_status))
    return S7E_ALREADY_STARTED;

  apr_status_t rv;

  // create communication pipes
  pipe_t* cmd_child = apr_pcalloc(pm->pool, sizeof(pipe_t));
  pipe_t* cmd_parent = apr_pcalloc(pm->pool, sizeof(pipe_t));

  rv = create_pipe_pair(pm->pool, cmd_child, cmd_parent);
  if (rv != APR_SUCCESS)
    return rv;

  apr_file_inherit_set(cmd_child->rd);
  apr_file_inherit_set(cmd_child->wr);

  // fork
  rv = apr_proc_fork(&pm->pm_proc, pm->pool);

  // child
  if (rv == APR_INCHILD) {
    // close parent side of command pipe
    // close_pipe(cmd_parent);
    pm->cmd_pipe = cmd_child;

    // run process manager
    exit(pm_main(pm));
  }
  // error
  else if (rv != APR_INPARENT) {
    return rv;
  }
  // parent
  else {
    // close child side of command pipe
    // close_pipe(cmd_child);
    pm->cmd_pipe = cmd_parent;

    // ...
    pm->pm_status = S7E_PROC_UP;

    // ...
    apr_pool_note_subprocess(pm->pool, &pm->pm_proc, APR_KILL_ONLY_ONCE);

    // This registers a handler for child notification. It'll be called
    // when either the associated pool is destroy or someone calls one
    // of apr_proc_other_child_{unregister,child_alert,refresh{,_all}}.
    //
    // All httpd mpm modules watch their children and run child_alert
    // when one dies.
    //
    // IMPORTANT: Will this also be called for children that are forked
    // in {pre,post}_config state? I don't think so.
    apr_proc_other_child_register(&pm->pm_proc, pm_maintenance, pm, NULL, pm->pool);
  }

  return APR_SUCCESS;
}

apr_status_t s7e_add_process(s7e_t* pm, const char* argv[]) {
  if (!S7E_PROC_IS_RUNNING(pm->pm_status))
    return S7E_NOT_RUNNING;

  CmdAdd cmd = CMD_ADD__INIT;

  size_t argv_bytes = 0;
  cmd.n_argv = 0;

  for (const char** arg = argv; *arg != NULL; arg++) {
    argv_bytes += strlen(*arg) + 2;
    cmd.n_argv++;
  }

  cmd.argv = argv;

  Msg msg = {
    PROTOBUF_C_MESSAGE_INIT(&msg__descriptor),
    MSG__TYPE_CMD_ADD,
    { &cmd }
  };

  return send_to_file((const ProtobufCMessage*) &msg, pm->cmd_pipe->wr);
}
