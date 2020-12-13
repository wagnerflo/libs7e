#include "s7e.h"
#include "s7e/pm.h"
#include "s7e/proto.h"
#include "cmd.pb-c.h"

apr_status_t s7e_add_process(s7e_t* pm, const char* argv[]) {
  if (!S7E_PROC_IS_RUNNING(pm->pm_status))
    return S7E_NOT_RUNNING;

  CmdAdd cmd = CMD_ADD__INIT;

  size_t argv_bytes = 0;
  cmd.n_argv = 0;

  for (const char** arg = argv; *arg != NULL; arg++) {
    argv_bytes += strlen(*arg) + 2;
    cmd.n_argv++;
  }

  cmd.argv = (char**) argv;

  Msg msg = {
    PROTOBUF_C_MESSAGE_INIT(&msg__descriptor),
    MSG__TYPE_CMD_ADD,
    { &cmd }
  };

  return send_to_file((const ProtobufCMessage*) &msg, pm->cmd_pipe->wr);
}
