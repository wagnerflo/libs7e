#include <stdlib.h>

#include "s7e/pm.h"
#include "s7e/pipe.h"

static void maintain_child(int reason, void* data, int status) {
  s7e_t* pm = (s7e_t*) data;

  switch (reason) {
    case APR_OC_REASON_DEATH:
      printf("pm_maintenance(APR_OC_REASON_DEATH, ..., %d)\n", status);
      break;

    case APR_OC_REASON_UNWRITABLE:
      printf("pm_maintenance(APR_OC_REASON_UNWRITABLE, ..., %d)\n", status);
      break;

    case APR_OC_REASON_RESTART:
      printf("pm_maintenance(APR_OC_REASON_RESTART, ..., %d)\n", status);
      break;

    // this happens on explicit apr_proc_other_child_unregister(data) as
    // well as when the pool, this process is register to is destroyed
    case APR_OC_REASON_UNREGISTER:
      printf("pm_maintenance(APR_OC_REASON_UNREGISTER, ..., %d)\n", status);
      // tell process manager to exit and wait for it
      apr_proc_kill(&pm->pm_proc, SIGHUP);
      apr_proc_wait(&pm->pm_proc, NULL, NULL, APR_WAIT);

      // clear runtime pool
      apr_pool_clear(pm->pool);

      break;

    case APR_OC_REASON_LOST:
      printf("pm_maintenance(APR_OC_REASON_LOST, ..., %d)\n", status);
      break;

    case APR_OC_REASON_RUNNING:
      printf("pm_maintenance(APR_OC_REASON_RUNNING, ..., %d)\n", status);
      break;
  }
}

static apr_status_t setup_shm(s7e_t* pm) {
  apr_status_t rv;
  apr_shm_t* shm;
  void* base;
  size_t sz = sizeof(uint32_t) * pm->max_proc;

  rv = apr_shm_create(&shm, sz, NULL, pm->pool);
  if (rv != APR_SUCCESS)
    return rv;

  if (apr_shm_size_get(shm) != sz) {
    apr_shm_destroy(shm);
    return APR_INCOMPLETE;
  }

  base = apr_shm_baseaddr_get(shm);
  memset(base, 0, sz);

  pm->fs_shm = shm;
  pm->fs_base = (uint32_t*) base;

  return APR_SUCCESS;
}

apr_status_t destroy_shm(void* data) {
  s7e_t* pm = (s7e_t*) data;

  if (pm->fs_shm != NULL) {
    apr_shm_destroy(pm->fs_shm);
    pm->fs_shm = NULL;
    pm->fs_base = NULL;
  }

  return APR_SUCCESS;
}

apr_status_t reset_s7e(void* data) {
  s7e_t* pm = (s7e_t*) data;
  pm->pm_status = S7E_PROC_DOWN;
  pm->cmd_pipe = NULL;
  return APR_SUCCESS;
}

apr_status_t s7e_start(s7e_t* pm) {
  if (S7E_PROC_IS_RUNNING(pm->pm_status))
    return S7E_ALREADY_STARTED;

  apr_status_t rv;

  // create runtime pool
  if (pm->pool == NULL) {
    rv = apr_pool_create(&pm->pool, pm->parent_pool);
    if (rv != APR_SUCCESS)
      return rv;
  }

  // create shared memory
  if (pm->fs_enabled) {
    rv = setup_shm(pm);
    if (rv != APR_SUCCESS) {
      apr_pool_clear(pm->pool);
      return rv;

    }
    apr_pool_cleanup_register(
        pm->pool, pm, destroy_shm, apr_pool_cleanup_null);
  }

  // create communication pipes
  pipe_t* cmd_child = apr_pcalloc(pm->pool, sizeof(pipe_t));
  pipe_t* cmd_parent = apr_pcalloc(pm->pool, sizeof(pipe_t));

  if (cmd_child == NULL || cmd_parent == NULL) {
    apr_pool_clear(pm->pool);
    return APR_ENOMEM;
  }

  rv = pipe_create_pair(pm->pool, cmd_child, cmd_parent);
  if (rv != APR_SUCCESS) {
    apr_pool_clear(pm->pool);
    return rv;
  }

  // on fork inherit child pipe, ...
  rv = pipe_inherit_set(cmd_child);
  if (rv != APR_SUCCESS) {
    apr_pool_clear(pm->pool);
    return rv;
  }

  // ... but not parent
  pipe_inherit_unset(cmd_parent);
  if (rv != APR_SUCCESS) {
    apr_pool_clear(pm->pool);
    return rv;
  }

  // fork
  rv = apr_proc_fork(&pm->pm_proc, pm->pool);

  // child
  if (rv == APR_INCHILD) {
    // any further children should not inherit the command pipe
    pipe_inherit_unset(cmd_child);

    // setup p7e handle for parent/process manager use; set a special
    // status so we can guard against hooks trying to use this handle to
    // configure/start another process manager
    pm->pm_status = S7E_PROC_IS_PM;
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
    pipe_close(cmd_child);
    pm->cmd_pipe = cmd_parent;

    // setup p7e handle for parent use
    pm->pm_status = S7E_PROC_UP;

    // register struct reset on runtime pool clear/destroy
    apr_pool_cleanup_register(
        pm->pool, pm, reset_s7e, apr_pool_cleanup_null);

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
    // in pre_config hook? I don't think so.
    apr_proc_other_child_register(&pm->pm_proc, maintain_child, pm, NULL, pm->pool);
  }

  return APR_SUCCESS;
}

apr_status_t s7e_stop(s7e_t* pm) {
  if (!S7E_PROC_IS_RUNNING(pm->pm_status))
    return S7E_NOT_RUNNING;

  apr_proc_other_child_unregister(pm);
  return APR_SUCCESS;
}
