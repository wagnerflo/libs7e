#include <apr_signal.h>
#include <apr_poll.h>

#include "s7e/pm.h"
#include "s7e/pipe.h"

static pipe_t selfpipe = { NULL, NULL };

static void signal_to_pipe(int signo) {
  //apr_file_putc(signo, selfpipe.wr);
  apr_file_write_full(selfpipe.wr, &signo, sizeof(int), NULL);
}

static apr_status_t pm_handle_signal(
    __attribute__((unused)) pm_t* pm,
    const apr_pollfd_t* pfd) {

  int signo;
  apr_file_read_full(pfd->desc.f, &signo, sizeof(int), NULL);
  printf("signal = %d\n", signo);

  switch (signo) {
    case SIGHUP:
    case SIGTERM:
      return S7E_SIGNALED_EXIT;
  }

  return APR_SUCCESS;
}

apr_status_t pm_setup_signals(pm_t* pm) {
  apr_status_t rv;

  // create self-pipe
  rv = apr_file_pipe_create_ex(
      &selfpipe.rd, &selfpipe.wr, APR_FULL_NONBLOCK, pm->shared->pool);
  if (rv != APR_SUCCESS)
    return rv;

  // don't inherit to children
  rv = pipe_inherit_unset(&selfpipe);
  if (rv != APR_SUCCESS)
    return rv;

  // add read end to pollset
  apr_pollfd_t pfd = {
    pm->shared->pool,
    APR_POLL_FILE,
    APR_POLLIN,
    0,
    { selfpipe.rd },
    &pm_handle_signal
  };

  rv = apr_pollset_add(pm->pollset, &pfd);
  if (rv != APR_SUCCESS)
    return rv;

  apr_signal(SIGHUP,  &signal_to_pipe);
  apr_signal(SIGTERM, &signal_to_pipe);
  apr_signal(SIGCHLD, &signal_to_pipe);

  return APR_SUCCESS;
}
