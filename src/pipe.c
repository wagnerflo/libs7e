#include "s7e.h"
#include "s7e/pipe.h"

#include <apr_pools.h>
#include <apr_file_io.h>

apr_status_t close_pipe(pipe_t* pipe) {
  apr_status_t rv0 = APR_SUCCESS;
  apr_status_t rv1 = APR_SUCCESS;

  rv0 = apr_file_close(pipe->rd);
  rv1 = apr_file_close(pipe->wr);

  pipe->rd = NULL;
  pipe->wr = NULL;

  return rv0 ? rv0 : rv1;
}

apr_status_t create_selfpipe(apr_pool_t* pool, pipe_t* pipe) {
  if (pipe->rd != NULL || pipe->wr != NULL)
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

  pipe->rd = rp;
  pipe->wr = wp;

  return APR_SUCCESS;
}

apr_status_t create_pipe_pair(apr_pool_t* pool, pipe_t* sa, pipe_t* sb) {
  apr_status_t rv;
  apr_file_t* r1;
  apr_file_t* w1;
  apr_file_t* r2;
  apr_file_t* w2;

  rv = apr_file_pipe_create(&r1, &w1, pool);
  if (rv != APR_SUCCESS)
    return rv;

  rv = apr_file_pipe_create(&r2, &w2, pool);
  if (rv != APR_SUCCESS)
    return rv;

  sa->rd = r1;
  sa->wr = w2;

  sb->rd = r2;
  sb->wr = w1;

  return APR_SUCCESS;
}