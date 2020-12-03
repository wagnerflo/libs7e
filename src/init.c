#include <stdio.h>
#include <apr_strings.h>

#include <apr_general.h>
#include <apr_pools.h>
#include <apr_thread_proc.h>

#include "s7e.h"
#include "s7e/pm.h"
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

  // fork
  apr_status_t rv = apr_proc_fork(&pm->pm_proc, pm->pool);

  // child
  if (rv == APR_INCHILD) {
    pm_main(pm);
  }
  // error
  else if (rv != APR_INPARENT) {
    return rv;
  }
  // parent
  else {
    pm->pm_status = S7E_PROC_UP;
    apr_pool_note_subprocess(pm->pool, &pm->pm_proc, APR_KILL_ONLY_ONCE);
    apr_proc_other_child_register(&pm->pm_proc, run_maintenance, pm, NULL, pm->pool);
  }

  return APR_SUCCESS;
}

apr_status_t s7e_add_process(s7e_t* pm, const char* argv[]) {
  if (!S7E_PROC_IS_RUNNING(pm->pm_status))
    return S7E_NOT_RUNNING;

  AddCommand cmd = ADD_COMMAND__INIT;

  size_t argv_bytes = 0;
  cmd.n_argv = 0;

  for (const char** arg = argv; *arg != NULL; arg++) {
    argv_bytes += strlen(*arg) + 2;
    cmd.n_argv++;
  }

  cmd.argv = argv;

  Command c = {
    PROTOBUF_C_MESSAGE_INIT(&command__descriptor),
    COMMAND__TYPE_ADD,
    { &cmd }
  };

  unsigned len = command__get_packed_size(&c);
  void* buf = malloc(len);
  command__pack(&c, buf);

  Command* x = command__unpack(NULL, len, buf);

  switch (x->type_case) {
    case COMMAND__TYPE_ADD:
      printf("add\n");
      for (unsigned int i = 0; i < x->add->n_argv; i++) {
        printf("%d = %s\n", i, x->add->argv[i]);
      }
      break;

    case COMMAND__TYPE_REMOVE:
      printf("remove\n");
      break;
  }

  command__free_unpacked(x, NULL);

  return APR_SUCCESS;
}
