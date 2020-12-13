#include <apr_pools.h>
#include <apr_shm.h>
#include <apr_time.h>
#include <apr_thread_proc.h>
#include <stdatomic.h>

int main() {
  apr_status_t rv;
  apr_pool_t* pool;
  apr_shm_t* shm;
  apr_proc_t proc;
  size_t sz = sizeof(int) * 32;
  void* base = NULL;
  int* as_int = NULL;

  rv = apr_pool_initialize();
  if (rv != APR_SUCCESS)
    return 1;

  rv = apr_pool_create(&pool, NULL);
  if (rv != APR_SUCCESS)
    return 1;

  rv = apr_shm_create(&shm, sz, NULL, pool);
  if (rv != APR_SUCCESS)
    return 1;

  if (apr_shm_size_get(shm) != sz)
    return 1;

  base = apr_shm_baseaddr_get(shm);

  if (atomic_is_lock_free(base) != 1)
    return 1;

  as_int = (int*) base;

  if (atomic_is_lock_free(as_int) != 1)
    return 1;

  for (int i = 0; i < 32; i++)
    atomic_store(as_int + i, 0);

  for (int i = 0; i < 32; i++)
    if (atomic_load(as_int + i) != 0)
      return 1;

  rv = apr_proc_fork(&proc, pool);

  // child
  if (rv == APR_INCHILD) {
    apr_sleep(100000);
    for (int i = 0; i < 32; i++)
      atomic_store(as_int + i, i + 100);
  }
  // error
  else if (rv != APR_INPARENT) {
    return rv;
  }
  // parent
  else {
    int val;
    for (int i = 0; i < 32; i++) {
      while ((val = atomic_load(as_int + i)) == 0)
        apr_sleep(10000);
      if (val != i + 100)
        return 1;
    }
  }

  rv = apr_shm_destroy(shm);
  if (rv != APR_SUCCESS)
    return 1;

  apr_pool_destroy(pool);

  return 0;
}
