#include <stdio.h>
#include <apr_pools.h>
#include <apr_time.h>
#include <s7e.h>
#include <s7e/bitset.h>

apr_status_t pre_spawn(apr_pool_t* pool, s7e_t* pm) {

}

int main() {
  apr_status_t rv;
  apr_pool_t* pool;
  const char* cmd[] = { "/home/wagnerflo/coding/libs7e/child1.sh", "lol", NULL };

  apr_pool_initialize();
  apr_pool_create(&pool, NULL);

  apr_time_t now = apr_time_now();
  char* x = apr_pcalloc(pool, APR_RFC822_DATE_LEN * sizeof(char));
  apr_rfc822_date(x, now);
  printf("%d %s\n", apr_time_sec(now), x);

  s7e_t* pm = s7e_create(pool);
  if (pm == NULL)
    return 1;
  rv = s7e_enable_fast_status(pm);
  printf("s7e_enable_fast_status() -> %d\n", rv);
  rv = s7e_start(pm);
  printf("s7e_start() -> %d\n", rv);
  rv = s7e_add_process(pm, cmd);
  printf("s7e_add_process() -> %d\n", rv);

  apr_sleep(5 * 1000000);
  //s7e_stop(pm);

  printf("apr_pool_destroy {\n");
  apr_pool_destroy(pool);
  printf("}\n");

  return 0;
}
