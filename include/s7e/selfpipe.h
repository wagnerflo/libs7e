#ifndef S7E_SELFPIPE_H
#define S7E_SELFPIPE_H

#include <apr_portable.h>

apr_status_t create_selfpipe(apr_pool_t*, apr_file_t**, apr_file_t**);

#endif /* S7E_SELFPIPE_H */
