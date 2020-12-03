#include "s7e.h"

#include <apr_pools.h>
#include <apr_file_io.h>

apr_status_t create_selfpipe(apr_pool_t* pool,
                             apr_file_t** sig_read,
                             apr_file_t** sig_write) {
  if (*sig_read != NULL || *sig_write != NULL)
    return S7E_SELFPIPE_EXISTS;

  apr_status_t rv;
  apr_file_t* rp;
  apr_file_t* wp;

  rv = apr_file_pipe_create_ex(&rp, &wp, APR_FULL_NONBLOCK, pool);
  if (rv != APR_SUCCESS)
    return rv;

  rv = apr_file_inherit_unset(rp);
  if (rv != APR_SUCCESS)
    return rv;

  rv = apr_file_inherit_unset(wp);
  if (rv != APR_SUCCESS)
    return rv;

  *sig_read = rp;
  *sig_write = wp;

  return APR_SUCCESS;
}
