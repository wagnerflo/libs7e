#include "s7e.h"
#include "s7e/pm.h"

#include <stdio.h>
#include <apr_strings.h>

void run_maintenance(int reason, void* data, int status) {
  printf("run_maintenance(%d, ..., %d)\n", reason, status);
}

apr_status_t pm_signal_handler(int signo) {

}

apr_status_t pm_setup_signals() {
  struct sigaction sa;
}

void pm_setup_pipe() {

}

apr_status_t pm_setup_fast_status() {

}

apr_status_t pm_spawn_process(s7e_t* pm) {
  apr_proc_t proc;
  apr_procattr_t* procattr;
  const char* cmd[] = { "/home/wagner/p/libs7e/child1.sh", NULL };

  apr_procattr_create(&procattr, pm->pool);
  apr_procattr_cmdtype_set(procattr, APR_PROGRAM);

  return apr_proc_create(&proc, cmd[0], cmd, NULL, procattr, pm->pool);
}

void pm_main(s7e_t* pm) {
  // set a special status so we can guard against hooks trying to use
  // this pm struct to configure/start another process manager
  pm->pm_status = S7E_PROC_IS_PM;

  pm_setup_signals();
  pm_setup_pipe();
  pm_setup_fast_status();
  //pm_spawn_process(pm);

  while (1) {

  }
}
