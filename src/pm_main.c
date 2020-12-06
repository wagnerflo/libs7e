#include "s7e.h"
#include "s7e/pm.h"
#include "cmd.pb-c.h"

#include <apr_poll.h>

#include <stdio.h>
#include <apr_strings.h>

apr_status_t pm_spawn_process(s7e_t* pm) {
  apr_proc_t proc;
  apr_procattr_t* procattr;
  const char* cmd[] = { "/home/wagnerflo/coding/libs7e/child1.sh", NULL };

  apr_procattr_create(&procattr, pm->pool);
  apr_procattr_cmdtype_set(procattr, APR_PROGRAM);

  apr_proc_create(&proc, cmd[0], cmd, NULL, procattr, pm->pool);

  return APR_SUCCESS;
}

apr_status_t pm_handle_cmd(pm_t* pm, const apr_pollfd_t* pfd) {
  printf("pm_handle_cmd\n");

  // read message length
  uint32_t msglen;
  apr_file_read_full(pfd->desc.f, &msglen, sizeof(uint32_t), NULL);
  msglen = ntohl(msglen);

  // allocate and fill data buffer
  uint8_t* msgbuf = apr_palloc(pm->handler_pool, msglen);
  apr_file_read_full(pfd->desc.f, msgbuf, msglen, NULL);

  // unpack message
  Msg* msg = msg__unpack(NULL, msglen, msgbuf);

  switch (msg->type_case) {
    case MSG__TYPE_CMD_ADD:
      printf("add\n");
      for (unsigned int i = 0; i < msg->cmd_add->n_argv; i++) {
        printf("%d = %s\n", i, msg->cmd_add->argv[i]);
      }
      pm_spawn_process(pm->shared);
      break;

    case MSG__TYPE_CMD_REMOVE:
      printf("remove\n");
      break;

    default:
      printf("BAD\n");
      break;
  }

  msg__free_unpacked(msg, NULL);

  return APR_SUCCESS;
}

apr_status_t pm_main(s7e_t* shared) {
  apr_status_t rv;

  // create process manager handle
  pm_t pm = {
    shared,
    NULL,
    NULL,
    NULL,
    NULL,
  };

  // create pollset
  // IMPORTANT: second parameter is size: how to set it correctly?
  rv = apr_pollset_create(&pm.pollset, 2, shared->pool, 0);
  if (rv != APR_SUCCESS)
    return rv;

  // setup signal handlers and add to pollset
  rv = pm_setup_signals(&pm);
  if (rv != APR_SUCCESS)
    return rv;

  // add command pipe to pollset
  apr_pollfd_t pfd_cmd_pipe = {
    pm.shared->pool,
    APR_POLL_FILE,
    APR_POLLIN,
    0,
    { pm.shared->cmd_pipe->rd },
    &pm_handle_cmd
  };
  apr_pollset_add(pm.pollset, &pfd_cmd_pipe);

  // prepare memory pool for poll handlers
  apr_pool_create(&pm.handler_pool, pm.shared->pool);

  // main loop
  apr_status_t exit_rv = APR_SUCCESS;
  apr_int32_t num;
  const apr_pollfd_t* ret_pfd;

  while (exit_rv == APR_SUCCESS) {
    rv = apr_pollset_poll(pm.pollset, -1, &num, &ret_pfd);
    printf("poll -> rv=%d, num=%d\n", rv, num);

    for (int i = 0; i < num; i++) {
      const apr_pollfd_t* p = ret_pfd + i;
      pm_pollset_handler* handler = (pm_pollset_handler*) p->client_data;
      rv = handler(&pm, p);
      apr_pool_clear(pm.handler_pool);

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
