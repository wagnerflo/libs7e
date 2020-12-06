#include <limits.h>

int main() {
  unsigned int m = UINT_MAX, c = 0;
  do { c++; } while (m >>= 1);
  return c;
}
