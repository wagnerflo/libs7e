#include <stdlib.h>
#include "s7e/pm.h"

static int proc_actions_compare(void* v1, void* v2) {
  action_t* a1 = v1;
  action_t* a2 = v2;

  if (a1->when == a2->when)
    return  0;
  else if (a1->when < a2->when)
    return -1;
  else
    return  1;
}

apr_status_t pm_setup_actions(pm_t* pm) {
  apr_status_t rv;

  rv = apr_skiplist_init(&pm->proc_actions, pm->shared->pool);
  if (rv != APR_SUCCESS)
    return rv;

  apr_skiplist_set_compare(
      pm->proc_actions, proc_actions_compare, proc_actions_compare);

  return APR_SUCCESS;
}

apr_interval_time_t actions_poll_timeout(pm_t* pm) {
  action_t* action = apr_skiplist_peek(pm->proc_actions);

  if (action == NULL)
    return -1;

  apr_interval_time_t timeout = action->when - apr_time_now();
  if (timeout < 0)
    return 0;

  return timeout;
}

apr_status_t actions_handle_current(pm_t* pm, apr_pool_t* pool) {
  apr_time_t now = apr_time_now();
  action_t* action;

  while (1) {
    action = apr_skiplist_peek(pm->proc_actions);
    if (action == NULL || action->when > now)
      break;

    apr_skiplist_pop(pm->proc_actions, NULL);
    action->handler(pm, pool, action->data);
    free(action);
  }

  return APR_SUCCESS;
}

apr_status_t actions_add_now(pm_t* pm, action_handler* handler, void* data) {
  action_t* action = malloc(sizeof(action_t));
  if (action == NULL)
    return APR_ENOMEM;

  action->when = apr_time_now();
  action->handler = handler;
  action->data = data;

  apr_skiplist_insert(pm->proc_actions, action);

  return APR_SUCCESS;
}

apr_status_t action_start(pm_t* pm, __attribute__((unused)) apr_pool_t* pool, void* data) {
  proc_config_t* conf = data;

  printf("action_start\n");
  printf("  args:");
  for (char** arg = conf->argv; *arg != NULL; arg++) {
    printf(" %s", *arg);
  }
  printf("\n");

  apr_proc_t proc;
  apr_procattr_t* procattr;

  apr_procattr_create(&procattr, pm->shared->pool);
  apr_procattr_cmdtype_set(procattr, APR_PROGRAM);

  apr_proc_create(&proc, conf->argv[0], (const char * const*) conf->argv,
                  NULL, procattr, pm->shared->pool);

  return APR_SUCCESS;
}
