int main() {
  unsigned int val = 4;
  if (__builtin_clz(val) == UINT_BITS - 3)
    return 0;
  return 1;
}
