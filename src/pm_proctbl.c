#include <stdlib.h>
#include "s7e/pm.h"

apr_status_t pm_setup_proctbl(pm_t* pm) {
  apr_status_t rv;

  rv = bitset_init(pm->shared->pool, &pm->proc_map, pm->shared->max_proc);
  if (rv != APR_SUCCESS)
    return rv;

  pm->proc_config = apr_hash_make(pm->shared->pool);
  if (pm->proc_config == NULL)
    return APR_ENOMEM;

  return APR_SUCCESS;
}

apr_status_t proctbl_add(pm_t* pm, proc_config_t** conf) {
  apr_status_t rv;

  // find a free slot
  unsigned int slot = bitset_flip_any_zero(&pm->proc_map);
  if (slot == UINT_MAX)
    return S7E_MAX_PROCESSES;

  apr_pool_t* pool;

  rv = apr_pool_create(&pool, apr_hash_pool_get(pm->proc_config));
  if (rv != APR_SUCCESS) {
    bitset_unset(&pm->proc_map, slot);
    return rv;
  }

  proc_config_t* c = apr_pcalloc(pool, sizeof(proc_config_t));
  if (c == NULL) {
    apr_pool_destroy(pool);
    bitset_unset(&pm->proc_map, slot);
    return APR_ENOMEM;
  }

  unsigned int* key = malloc(sizeof(unsigned int));
  if (key == NULL) {
    apr_pool_destroy(pool);
    bitset_unset(&pm->proc_map, slot);
    return APR_ENOMEM;
  }

  c->key = key;
  c->pool = pool;
  *key = slot;
  *conf = c;

  apr_hash_set(pm->proc_config, c->key, sizeof(unsigned int), c);

  return APR_SUCCESS;
}

apr_status_t proctbl_remove(pm_t* pm, unsigned int slot) {
  proc_config_t* c = apr_hash_get(pm->proc_config, &slot, sizeof(unsigned int));
  if (c == NULL)
    return APR_NOTFOUND;

  apr_hash_set(pm->proc_config, c->key, sizeof(unsigned int), NULL);
  bitset_unset(&pm->proc_map, slot);
  free(c->key);
  apr_pool_destroy(c->pool);

  return APR_SUCCESS;
}
