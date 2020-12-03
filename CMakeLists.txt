cmake_minimum_required( VERSION 3.14 )
project ( libs7e )

find_package( PkgConfig REQUIRED )

pkg_check_modules( APR1      REQUIRED IMPORTED_TARGET apr-1 )
pkg_check_modules( PROTOBUFC REQUIRED IMPORTED_TARGET libprotobuf-c )

find_library ( PROTOBUFC_LIBRARIES_STATIC NAMES libprotobuf-c.a )

add_custom_command(
    OUTPUT src/proto/cmd.pb-c.c src/proto/cmd.pb-c.h
    COMMAND protoc-c --c_out=../src/proto cmd.proto
    MAIN_DEPENDENCY proto/cmd.proto
    WORKING_DIRECTORY proto
)

add_library( s7e SHARED
    src/init.c
    src/pm.c
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

add_executable ( test test.c )
target_include_directories ( test PUBLIC include )
target_link_libraries( test PUBLIC s7e )
