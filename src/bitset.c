#include "s7e/bitset.h"

#define UINT(val) ((unsigned int) val)
#define WORD_OFFSET(bit) (bit / UINT_BITS)
#define BIT_OFFSET(bit) (bit % UINT_BITS)

apr_status_t bitset_create(apr_pool_t* pool, bitset_t** bitset, unsigned int bits) {
  if (bits == 0 || bits == UINT_MAX)
    return APR_EINVAL;

  bitset_t* b = apr_pcalloc(pool, sizeof(bitset_t));
  if (b == NULL)
    return APR_ENOMEM;

  apr_status_t rv = bitset_init(pool, b, bits);
  if (rv != APR_SUCCESS)
    return rv;

  *bitset = b;

  return APR_SUCCESS;
}

apr_status_t bitset_init(apr_pool_t* pool, bitset_t* bitset, unsigned int bits) {
  if (bits == 0)
    return APR_EINVAL;

  unsigned int offset = BIT_OFFSET(bits);
  unsigned int num_words = WORD_OFFSET(bits) + (offset != 0);

  bitset->words = apr_pcalloc(pool, sizeof(unsigned int) * num_words);
  if (bitset->words == NULL)
    return APR_ENOMEM;

  bitset->num_words = num_words;
  bitset->num_bits = bits;
  bitset->num_zeros = bits;

  if (offset != 0)
    bitset->words[num_words - 1] = ~UINT(0) << offset;

  return APR_SUCCESS;
}

static apr_status_t bitset_change(bitset_t* bitset, unsigned int bit, int val) {
  if (bit > bitset->num_bits)
    return APR_EINVAL;

  unsigned int word = bitset->words[WORD_OFFSET(bit)];
  unsigned int shift = UINT(1) << BIT_OFFSET(bit);

  // bit is set
  if (word & shift) {
    if (val >= 0) {
      bitset->num_zeros++;
      bitset->words[WORD_OFFSET(bit)] = word ^ shift;
    }
  }
  // bit is not set
  else {
    if (val <= 0) {
      bitset->num_zeros--;
      bitset->words[WORD_OFFSET(bit)] = word ^ shift;
    }
  }

  return APR_SUCCESS;
}

apr_status_t bitset_set(bitset_t* bitset, unsigned int bit) {
  return bitset_change(bitset, bit,  1);
}

apr_status_t bitset_unset(bitset_t* bitset, unsigned int bit) {
  return bitset_change(bitset, bit, -1);
}

apr_status_t bitset_flip(bitset_t* bitset, unsigned int bit) {
  return bitset_change(bitset, bit,  0);
}

#ifdef HAVE_BUILTIN_CLZ
#define clz(x) __builtin_clz(x)
#elif UINT_BITS == 32
static const int clz_de_bruijn[32] = {
  31, 22, 30, 21, 18, 10, 29, 2, 20, 17, 15, 13, 9, 6, 28, 1,
  23, 19, 11, 3, 16, 14, 7, 24, 12, 4, 8, 25, 5, 26, 27, 0
};
static int clz(uint32_t v) {
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  return clz_de_bruijn[(uint32_t)(v * 0x07C4ACDDU) >> 27];
}
#endif

unsigned int bitset_flip_any_zero(bitset_t* bitset) {
  if (bitset->num_zeros == 0)
    return UINT_MAX;

  while(bitset->words[bitset->free_word] == UINT_MAX) {
    if (++bitset->free_word == bitset->num_words)
      bitset->free_word = 0;
  }

  unsigned int word = bitset->words[bitset->free_word];
  unsigned int offset = 0;

  if (word != 0) {
    offset = UINT(UINT_BITS) - UINT(clz(word));
    if (offset == UINT_BITS)
      offset = UINT(UINT_BITS) - UINT(clz(~word)) - 1;
  }

  bitset->num_zeros--;
  bitset->words[bitset->free_word] = word | UINT(1) << offset;

  return bitset->free_word * UINT_BITS + offset;
}
