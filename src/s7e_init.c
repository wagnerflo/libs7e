#include "s7e.h"
#include "s7e/pm.h"

s7e_t* s7e_create(apr_pool_t* parent) {
  s7e_t* p = apr_pcalloc(parent, sizeof(s7e_t));
  if (p == NULL)
    return NULL;
  p->parent_pool = parent;
  p->max_proc = 2048;
  return p;
}

apr_status_t s7e_set_prespawn_hook(s7e_t* pm, s7e_pre_spawn_hook_t* hook) {
  if (pm->pm_status != S7E_PROC_DOWN)
    return S7E_ALREADY_STARTED;

  pm->pre_spawn = hook;

  return APR_SUCCESS;
}

apr_status_t s7e_set_max_proc(s7e_t* pm, unsigned int max_proc) {
  if (pm->pm_status != S7E_PROC_DOWN)
    return S7E_ALREADY_STARTED;

  pm->max_proc = max_proc;

  return APR_SUCCESS;
}

apr_status_t s7e_enable_fast_status(s7e_t* pm) {
  if (pm->pm_status != S7E_PROC_DOWN)
    return S7E_ALREADY_STARTED;

#ifndef S7E_HAS_FAST_STATUS
  return APR_ENOTIMPL;
#else
  pm->fs_enabled = true;
  return APR_SUCCESS;
#endif
}
