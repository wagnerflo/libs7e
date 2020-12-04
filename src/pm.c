#include "s7e.h"
#include "s7e/pm.h"
#include "s7e/pipe.h"
#include "cmd.pb-c.h"

#include <apr_signal.h>
#include <apr_poll.h>

#include <stdio.h>
#include <apr_strings.h>

static pipe_t signal_pipe = { NULL, NULL };

typedef apr_status_t (pm_pollset_handler)
  (s7e_t*, apr_pool_t*, apr_pollset_t*, const apr_pollfd_t*);

void pm_maintenance(int reason, void* data, int status) {
  s7e_t* pm = (s7e_t*) data;

  switch (reason) {
    case APR_OC_REASON_DEATH:
      printf("pm_maintenance(APR_OC_REASON_DEATH, ...)\n");
      // apr_proc_other_child_unregister(data);
      break;
    case APR_OC_REASON_UNWRITABLE:
      break;
    case APR_OC_REASON_RESTART:
      break;

    // this happens on explicit apr_proc_other_child_unregister(data) as
    // well as when the pool, this process is register to is destroyed
    case APR_OC_REASON_UNREGISTER:
      printf("pm_maintenance(APR_OC_REASON_UNREGISTER, ...)\n");
      kill(pm->pm_proc.pid, SIGHUP);
      break;

    case APR_OC_REASON_LOST:
      break;
    case APR_OC_REASON_RUNNING:
      break;
  }
}

void signal_to_pipe(int signo) {
  apr_file_putc(signo, signal_pipe.wr);
}

apr_status_t pm_setup_signals(apr_pool_t* pool) {
  apr_status_t rv;

  rv = create_selfpipe(pool, &signal_pipe);
  if (rv != APR_SUCCESS)
    return rv;

  apr_signal(SIGHUP,  &signal_to_pipe);
  apr_signal(SIGTERM, &signal_to_pipe);
  apr_signal(SIGCHLD, &signal_to_pipe);
}

apr_status_t pm_setup_fast_status() {

}

apr_status_t pm_handle_signal(
    s7e_t* pm, apr_pool_t* hpool, apr_pollset_t* pollset, const apr_pollfd_t* pfd) {
  char ch;
  apr_file_getc(&ch, pfd->desc.f);
  printf("signal = %d\n", (int) ch);

  switch (ch) {
    case SIGHUP:
    case SIGTERM:
      return S7E_SIGNALED_EXIT;
  }

  return APR_SUCCESS;
}

apr_status_t pm_handle_cmd(
    s7e_t* pm, apr_pool_t* hpool, apr_pollset_t* pollset, const apr_pollfd_t* pfd) {
  printf("pm_handle_cmd\n");

  // read message length
  uint32_t msglen;
  apr_file_read_full(pfd->desc.f, &msglen, sizeof(uint32_t), NULL);
  msglen = ntohl(msglen);

  // allocate and fill data buffer
  uint8_t* msgbuf = apr_palloc(hpool, msglen);
  apr_file_read_full(pfd->desc.f, msgbuf, msglen, NULL);

  // unpack message
  Msg* msg = msg__unpack(NULL, msglen, msgbuf);

  switch (msg->type_case) {
    case MSG__TYPE_CMD_ADD:
      printf("add\n");
      for (unsigned int i = 0; i < msg->cmd_add->n_argv; i++) {
        printf("%d = %s\n", i, msg->cmd_add->argv[i]);
      }
      pm_spawn_process(pm);
      break;

    case MSG__TYPE_CMD_REMOVE:
      printf("remove\n");
      break;
  }

  msg__free_unpacked(msg, NULL);

  return APR_SUCCESS;
}

apr_status_t pm_spawn_process(s7e_t* pm) {
  apr_proc_t proc;
  apr_procattr_t* procattr;
  const char* cmd[] = { "/home/wagnerflo/coding/libs7e/child1.sh", NULL };

  apr_procattr_create(&procattr, pm->pool);
  apr_procattr_cmdtype_set(procattr, APR_PROGRAM);

  apr_proc_create(&proc, cmd[0], cmd, NULL, procattr, pm->pool);
}

apr_status_t pm_main(s7e_t* pm) {
  // set a special status so we can guard against hooks trying to use
  // this pm struct to configure/start another process manager
  pm->pm_status = S7E_PROC_IS_PM;

  apr_status_t rv;

  rv = pm_setup_signals(pm->pool);
  if (rv != APR_SUCCESS)
    return rv;

  // create pollset
  apr_pollset_t* pollset;
  apr_pollset_create(&pollset, 1, pm->pool, APR_POLLSET_NOCOPY);

  // add self pipe read end to pollset
  apr_pollfd_t pfd_signal = {
    pm->pool,
    APR_POLL_FILE,
    APR_POLLIN,
    0,
    { signal_pipe.rd },
    &pm_handle_signal
  };
  apr_pollset_add(pollset, &pfd_signal);

  // add command pipe to pollset
  apr_pollfd_t pfd_cmd_pipe = {
    pm->pool,
    APR_POLL_FILE,
    APR_POLLIN,
    0,
    { pm->cmd_pipe->rd },
    &pm_handle_cmd
  };
  apr_pollset_add(pollset, &pfd_cmd_pipe);

  // prepare memory pool for poll handlers
  apr_pool_t* hpool;
  apr_pool_create(&hpool, pm->pool);

  // main loop
  apr_status_t exit_rv = APR_SUCCESS;
  apr_int32_t num;
  const apr_pollfd_t* ret_pfd;

  while (exit_rv == APR_SUCCESS) {
    rv = apr_pollset_poll(pollset, -1, &num, &ret_pfd);
    printf("poll -> rv=%d, num=%d\n", rv, num);

    for (int i = 0; i < num; i++) {
      const apr_pollfd_t* p = ret_pfd + i;
      pm_pollset_handler* handler = (pm_pollset_handler*) p->client_data;
      rv = handler(pm, hpool, pollset, p);
      apr_pool_clear(hpool);

      if (rv == S7E_SIGNALED_EXIT) {
        exit_rv = rv;
        break;
      }
    }
  }

  // cleanup
  // ...

  return exit_rv;
}
