cmake_minimum_required( VERSION 3.14 )
project ( libs7e )

# find required packages and libraries
find_package( PkgConfig REQUIRED )

pkg_check_modules( APR1      REQUIRED IMPORTED_TARGET apr-1 )
pkg_check_modules( PROTOBUFC REQUIRED IMPORTED_TARGET libprotobuf-c )

find_library ( PROTOBUFC_LIBRARIES_STATIC NAMES libprotobuf-c.a )

# custom checks
message ( STATUS "Checking for builtin ctz" )
try_run (
  HAVE_BUILTIN_CTZ_EXIT HAVE_BUILTIN_CTZ_COMPILED
  ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}/cmake/builtin_ctz.c
  CMAKE_FLAGS -DCOMPILE_DEFINITIONS=-Wall\ -Wconversion\ -Werror
  COMPILE_OUTPUT_VARIABLE HAVE_BUILTIN_CTZ_COMPILE_OUTPUT
)
if ( ${HAVE_BUILTIN_CTZ_EXIT} EQUAL 2 )
  set( HAVE_BUILTIN_CTZ 1 CACHE INTERNAL "Test HAVE_BUILTIN_CTZ" )
  message ( STATUS "  Found" )
else ()
  set( HAVE_BUILTIN_CTZ "" CACHE INTERNAL "Test HAVE_BUILTIN_CTZ" )
  if ( NOT HAVE_BUILTIN_CTZ_COMPILED )
    file( APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
      "Performing SOURCE FILE Test HAVE_BUILTIN_CTZ_EXIT failed with the following output:\n"
      "${HAVE_BUILTIN_CTZ_COMPILE_OUTPUT}\n" )
    message ( STATUS "  Error compiling helper" )
  else ()
    message ( STATUS "  Wrong result running helper" )
  endif()
endif()

message ( STATUS "Determining number of bits in unsigned int" )
try_run(
  UINT_BITS UINT_BITS_COMPILED
  ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}/cmake/uint_bits.c
)
if( NOT UINT_BITS_COMPILED )
  message ( FATAL_ERROR "couldn't compile helper" )
endif()
message( STATUS "  Found ${UINT_BITS} bits in unsigned int")

# generate protobuf header and implementation
add_custom_command(
    OUTPUT src/proto/cmd.pb-c.c src/proto/cmd.pb-c.h
    COMMAND protoc-c --c_out=../src/proto cmd.proto
    MAIN_DEPENDENCY proto/cmd.proto
    WORKING_DIRECTORY proto
)

# generate config header
configure_file ( include/s7e/bitset.h.in include/s7e/bitset.h )

add_library( s7e SHARED
    src/s7e_init.c
    src/pm_main.c
    src/pm_signal.c
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
