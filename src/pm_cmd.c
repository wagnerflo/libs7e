#include <apr_poll.h>
#include "s7e/pm.h"
#include "cmd.pb-c.h"

static apr_status_t pm_spawn_process(s7e_t* pm) {
  apr_proc_t proc;
  apr_procattr_t* procattr;
  const char* cmd[] = { "/home/wagnerflo/coding/libs7e/child1.sh", NULL };

  apr_procattr_create(&procattr, pm->pool);
  apr_procattr_cmdtype_set(procattr, APR_PROGRAM);

  apr_proc_create(&proc, cmd[0], cmd, NULL, procattr, pm->pool);

  return APR_SUCCESS;
}

static apr_status_t pm_handle_cmd(pm_t* pm, const apr_pollfd_t* pfd) {
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

apr_status_t pm_setup_cmd(pm_t* pm) {
  apr_status_t rv;

  // add read end to pollset
  apr_pollfd_t pfd = {
    pm->shared->pool,
    APR_POLL_FILE,
    APR_POLLIN,
    0,
    { pm->shared->cmd_pipe->rd },
    &pm_handle_cmd
  };

  rv = apr_pollset_add(pm->pollset, &pfd);
  if (rv != APR_SUCCESS)
    return rv;

  return APR_SUCCESS;
}
