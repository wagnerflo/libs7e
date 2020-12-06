#include <protobuf-c/protobuf-c.h>
#include <apr_poll.h>

typedef ProtobufCBuffer pcb;
typedef ProtobufCMessage pcm;

typedef struct { pcb base; apr_file_t* file; } apr_file_buffer_t;
typedef struct { pcb base; apr_socket_t* sock; } apr_socket_buffer_t;

static void append_to_file(pcb* buf, size_t len, const uint8_t* data) {
  printf("writing to file/pipe\n");
  apr_file_write_full(((apr_file_buffer_t*) buf)->file, data, len, NULL);
}

static void append_to_socket(pcb* buf, size_t len, const uint8_t* data) {
  printf("writing to socket\n");
  apr_size_t size = len;
  apr_socket_send(((apr_socket_buffer_t*) buf)->sock, (const char*) data, &size);
}

static apr_status_t send_msg(const pcm* msg, pcb* buf) {
  uint32_t msglen = htonl((uint32_t) protobuf_c_message_get_packed_size(msg));
  buf->append(buf, sizeof(uint32_t), (const uint8_t*) &msglen);
  protobuf_c_message_pack_to_buffer(msg, buf);
  return APR_SUCCESS;
}

apr_status_t send_to_file(const pcm* msg, apr_file_t* file) {
  apr_file_buffer_t buf = { { append_to_file }, file };
  return send_msg(msg, (pcb*) &buf);
}

apr_status_t send_to_socket(const pcm* msg, apr_socket_t* sock) {
  apr_socket_buffer_t buf = { { append_to_socket }, sock };
  return send_msg(msg, (pcb*) &buf);
}

apr_status_t send_to_pollfd(const pcm* msg, apr_pollfd_t* pfd) {
  switch (pfd->desc_type) {
    case APR_POLL_FILE:   return send_to_file(msg, pfd->desc.f);
    case APR_POLL_SOCKET: return send_to_socket(msg, pfd->desc.s);
    default:              return APR_ENOTIMPL;
  }
  return APR_SUCCESS;
}

void recv_from_file() {

}
