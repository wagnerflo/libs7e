cmake_minimum_required( VERSION 3.14 )
project ( libs7e )

# find required packages and libraries
find_package( PkgConfig REQUIRED )

pkg_check_modules( APR1      REQUIRED IMPORTED_TARGET apr-1 )
pkg_check_modules( PROTOBUFC REQUIRED IMPORTED_TARGET libprotobuf-c )

find_library ( PROTOBUFC_LIBRARIES_STATIC NAMES libprotobuf-c.a )

# custom checks
include( config.d/CMakeLists.txt.inc )

# generate protobuf header and implementation
add_custom_command(
  OUTPUT src/proto/cmd.pb-c.c src/proto/cmd.pb-c.h
  COMMAND protoc-c --c_out=../src/proto cmd.proto
  MAIN_DEPENDENCY proto/cmd.proto
  WORKING_DIRECTORY proto
)

# generate config headers
configure_file ( include/s7e/bitset.h.in include/s7e/bitset.h )
configure_file ( include/s7e.h.in include/s7e.h )

add_library( s7e SHARED
  src/s7e_init.c
  src/s7e_run.c
  src/s7e_cmd.c
  src/pm_main.c
  src/pm_cmd.c
  src/pm_proctbl.c
  src/pm_signal.c
  src/bitset.c
  src/pipe.c
  src/proto.c
  src/proto/cmd.pb-c.c
  src/proto/cmd.pb-c.h
)
set( s7e_VERSION_SCRIPT src/libs7e.ver )
set_target_properties( s7e PROPERTIES
  LINK_DEPENDS ${s7e_VERSION_SCRIPT}
)
target_link_options( s7e PRIVATE
  -Wl,--version-script=${s7e_VERSION_SCRIPT}
  ${PROTOBUFC_LDFLAGS_OTHER}
)
target_compile_options( s7e PUBLIC
  ${PROTOBUFC_CFLAGS}
)
target_compile_options( s7e PRIVATE
  -Wall -Wextra -Wconversion -Werror
)
target_include_directories ( s7e PUBLIC
  include src/proto
)
target_link_libraries( s7e PUBLIC
  PkgConfig::APR1
)
target_link_libraries( s7e PRIVATE
  ${PROTOBUFC_LIBRARIES_STATIC}
)
target_compile_features( s7e PUBLIC c_std_11 )

add_executable ( test test.c )
target_include_directories ( test PUBLIC include )
target_link_libraries( test PUBLIC s7e )
