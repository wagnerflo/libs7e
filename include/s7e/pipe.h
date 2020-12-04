#ifndef S7E_PIPE_H
#define S7E_PIPE_H

#include <apr_portable.h>

struct pipe {
    apr_file_t* rd;
    apr_file_t* wr;
};

typedef struct pipe pipe_t;

apr_status_t close_pipe(pipe_t*);
apr_status_t create_selfpipe(apr_pool_t*, pipe_t*);
apr_status_t create_pipe_pair(apr_pool_t*, pipe_t*, pipe_t*);

#endif /* S7E_PIPE_H */
