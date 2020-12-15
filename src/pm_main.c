#include "s7e.h"
#include "s7e/pm.h"

#include <apr_poll.h>

apr_status_t pm_main(s7e_t* shared) {
  apr_status_t rv;

  // create process manager handle
  pm_t pm = {
    shared,
    NULL,
    NULL,
    EMPTY_BITSET,
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

  // setup command pipe and add to pollset
  rv = pm_setup_cmd(&pm);
  if (rv != APR_SUCCESS)
    return rv;

  // setup process management
  rv = pm_setup_proctbl(&pm);
  if (rv != APR_SUCCESS)
    return rv;

  rv = pm_setup_actions(&pm);
  if (rv != APR_SUCCESS)
    return rv;

  // prepare memory pool for poll handlers
  apr_pool_create(&pm.handler_pool, pm.shared->pool);

  // main loop
  apr_int32_t num_pfd;
  const apr_pollfd_t* ret_pfd;

  while (1) {
    printf("poll(timeout=%ld)\n", actions_poll_timeout(&pm));
    rv = apr_pollset_poll(pm.pollset, actions_poll_timeout(&pm),
                          &num_pfd, &ret_pfd);

    for (int i = 0; i < num_pfd; i++) {
      const apr_pollfd_t* pfd = ret_pfd + i;
      pm_pollset_handler* handler = (pm_pollset_handler*) pfd->client_data;
      rv = handler(&pm, pfd);

      if (rv == S7E_SIGNALED_EXIT)
        return rv;
    }

    rv = actions_handle_current(&pm, pm.handler_pool);
    if (rv == S7E_SIGNALED_EXIT)
      return rv;

    apr_pool_clear(pm.handler_pool);
  }

  // should never be reached
  return APR_EGENERAL;
}
