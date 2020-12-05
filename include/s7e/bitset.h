#ifndef S7E_BITSET_H
#define S7E_BITSET_H

#include "s7e/uint_bits.h"

typedef struct {
    apr_file_t* rd;
    apr_file_t* wr;
} bitset_t;

apr_status_t pipe_create_pair(apr_pool_t*, pipe_t*, pipe_t*);

apr_status_t pipe_close(pipe_t*);
apr_status_t pipe_inherit_set(pipe_t*);
apr_status_t pipe_inherit_unset(pipe_t*);

#endif /* S7E_BITSET_H */
