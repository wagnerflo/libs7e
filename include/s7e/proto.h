#ifndef S7E_PROTO_H
#define S7E_PROTO_H

#include <apr_poll.h>
#include <protobuf-c/protobuf-c.h>

apr_status_t send_to_file(const ProtobufCMessage*, apr_file_t*);
apr_status_t send_to_socket(const ProtobufCMessage*, apr_socket_t*);
apr_status_t send_to_pollfd(const ProtobufCMessage*, apr_pollfd_t*);



#endif /* S7E_PROTO_H */
