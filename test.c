#include <apr_pools.h>
#include <apr_time.h>
#include <s7e.h>

apr_status_t pre_spawn(apr_pool_t* pool, s7e_t* pm) {

}

int main() {
  apr_status_t rv;
  apr_pool_t* pool;
  const char* cmd[] = { "/home/wagner/p/libs7e/child1.sh", "lol", NULL };

  apr_pool_initialize();
  apr_pool_create(&pool, NULL);

  s7e_t* pm = s7e_init(pool);
  rv = s7e_start(pm);
  printf("s7e_start() -> %d\n", rv);
  rv = s7e_add_process(pm, cmd);
  printf("s7e_add_process() -> %d\n", rv);

  /* while (1) { */
  /*   struct s7e_event* evt = s7e_wait(pool, pm); */
  /* } */
  apr_sleep(5 * 1000000);

  printf("apr_pool_destroy {\n");
  apr_pool_destroy(pool);
  printf("}\n");

  return 0;
}
