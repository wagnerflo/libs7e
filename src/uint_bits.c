#include <limits.h>
#include <stdio.h>

int main() {
  unsigned int m = UINT_MAX, c = 0;
  do { c++; } while (m >>= 1);
  printf("#define UINT_BITS %u\n", c);
}
