#include "s7e.h"
#include "s7e/pm.h"
#include "s7e/selfpipe.h"

#include <apr_signal.h>
#include <apr_poll.h>

#include <stdio.h>
#include <apr_strings.h>

static apr_file_t* sig_read = NULL;
static apr_file_t* sig_write = NULL;

void run_maintenance(int reason, void* data, int status) {
  printf("run_maintenance(%d, ..., %d)\n", reason, status);
}


void pm_signal_handler(int signo) {
  apr_file_putc(signo, sig_write);
}

apr_status_t pm_setup_signals(apr_pool_t* pool) {
  apr_status_t rv;

  rv = create_selfpipe(pool, &sig_read, &sig_write);
  if (rv != APR_SUCCESS)
    return rv;

  apr_signal(SIGCHLD, &pm_signal_handler);
}

void pm_setup_pipe() {

}

apr_status_t pm_setup_fast_status() {

}

apr_status_t pm_spawn_process(s7e_t* pm) {
  apr_proc_t proc;
  apr_procattr_t* procattr;
  const char* cmd[] = { "/home/wagnerflo/coding/libs7e/child1.sh", NULL };

  apr_procattr_create(&procattr, pm->pool);
  apr_procattr_cmdtype_set(procattr, APR_PROGRAM);

  apr_proc_create(&proc, cmd[0], cmd, NULL, procattr, pm->pool);
}

#define POLL_SIGNAL   1
#define POLL_LISTEN   2
#define POLL_CMD_PIPE 4
#define POLL_CMD_SOCK 3

apr_status_t pm_main(s7e_t* pm) {
  // set a special status so we can guard against hooks trying to use
  // this pm struct to configure/start another process manager
  pm->pm_status = S7E_PROC_IS_PM;

  apr_status_t rv;

  rv = pm_setup_signals(pm->pool);
  if (rv != APR_SUCCESS)
    return rv;

  pm_setup_pipe();
  pm_setup_fast_status();

  // create pollset
  apr_pollset_t* pollset;
  apr_pollfd_t pfd;
  apr_pollset_create(&pollset, 1, pm->pool, 0);

  // add self pipe read end to pollset
  pfd.reqevents = APR_POLLIN;
  pfd.desc_type = APR_POLL_FILE;
  pfd.desc.f = sig_read;
  pfd.client_data = (void*) POLL_SIGNAL;
  apr_pollset_add(pollset, &pfd);

  // add command
  pm_spawn_process(pm);

  // main loop
  apr_int32_t num;
  const apr_pollfd_t* ret_pfd;
  char ch;
  while (1) {
    rv = apr_pollset_poll(pollset, -1, &num, &ret_pfd);
    printf("poll -> rv=%d, num=%d\n", rv, num);

    for (int i = 0; i < num; i++) {
      const apr_pollfd_t* p = ret_pfd + i;

      switch ((long) p->client_data) {
        case POLL_SIGNAL:
          apr_file_getc(&ch, p->desc.f);
          printf("signal = %d\n", (int) ch);
          break;
      }
    }
  }
}
