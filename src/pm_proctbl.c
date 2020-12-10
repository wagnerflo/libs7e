#include "s7e/pm.h"

apr_status_t pm_setup_proctbl(pm_t* pm) {
  apr_status_t rv;

  rv = bitset_init(pm->shared->pool, &pm->proc_map, pm->shared->max_proc);
  if (rv != APR_SUCCESS)
    return rv;

  return APR_SUCCESS;
}
