#include <apr_poll.h>
#include <apr_strings.h>
#include "s7e/pm.h"
#include "cmd.pb-c.h"

static apr_status_t cmd_add(pm_t* pm, CmdAdd* msg) {
  apr_status_t rv;
  proc_config_t* conf;

  rv = proctbl_add(pm, &conf);
  if (rv != APR_SUCCESS)
    return rv;

  conf->argv = apr_pcalloc(conf->pool, (msg->n_argv + 1) * sizeof(char*));
  if (conf->argv == NULL)
    return APR_ENOMEM;

  for (unsigned int i = 0; i < msg->n_argv; i++) {
    conf->argv[i] = apr_pstrdup(conf->pool, msg->argv[i]);
    if (conf->argv[i] == NULL)
      return APR_ENOMEM;
  }

  printf("cmd add\n");
  printf("  slot: %d\n", *conf->key);
  printf("  args:");
  for (char** arg = conf->argv; *arg != NULL; arg++) {
    printf(" %s", *arg);
  }
  printf("\n");

  actions_add_now(pm, action_start, conf);

  return APR_SUCCESS;
}

static apr_status_t pm_handle_cmd(pm_t* pm, const apr_pollfd_t* pfd) {
  apr_status_t rv;

  // read message length
  uint32_t msglen;
  rv = apr_file_read_full(pfd->desc.f, &msglen, sizeof(uint32_t), NULL);
  if (rv != APR_SUCCESS)
    return rv;

  msglen = ntohl(msglen);

  // allocate and fill data buffer
  uint8_t* msgbuf = apr_palloc(pm->handler_pool, msglen);
  if (msgbuf == NULL)
    return APR_ENOMEM;

  rv = apr_file_read_full(pfd->desc.f, msgbuf, msglen, NULL);
  if (rv != APR_SUCCESS)
    return rv;

  // unpack message
  Msg* msg = msg__unpack(NULL, msglen, msgbuf);

  switch (msg->type_case) {
    case MSG__TYPE_CMD_ADD:
      rv = cmd_add(pm, msg->cmd_add);
      break;

    case MSG__TYPE_CMD_REMOVE:
      printf("remove\n");
      break;

    default:
      printf("BAD\n");
      break;
  }

  msg__free_unpacked(msg, NULL);

  return rv;
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
