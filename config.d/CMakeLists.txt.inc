message ( STATUS "Determining number of bits in unsigned int" )
try_run(
  UINT_BITS UINT_BITS_COMPILED
  ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}/config.d/uint_bits.c
)
if( NOT UINT_BITS_COMPILED )
  message ( FATAL_ERROR "couldn't compile helper" )
endif()
message( STATUS "  Found ${UINT_BITS} bits in unsigned int")

message ( STATUS "Checking for builtin clz" )
try_run (
  HAVE_BUILTIN_CLZ_EXIT HAVE_BUILTIN_CLZ_COMPILED
  ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}/config.d/builtin_clz.c
  CMAKE_FLAGS -DCOMPILE_DEFINITIONS=-Wall\ -Wconversion\ -Werror\ -DUINT_BITS=${UINT_BITS}
  COMPILE_OUTPUT_VARIABLE HAVE_BUILTIN_CLZ_COMPILE_OUTPUT
)
if ( ${HAVE_BUILTIN_CLZ_EXIT} EQUAL 0 )
  set( HAVE_BUILTIN_CLZ 1 CACHE INTERNAL "Test HAVE_BUILTIN_CLZ" )
  message ( STATUS "  Found and working" )
else ()
  set( HAVE_BUILTIN_CLZ "" CACHE INTERNAL "Test HAVE_BUILTIN_CLZ" )
  if ( NOT HAVE_BUILTIN_CLZ_COMPILED )
    file( APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
      "Performing SOURCE FILE Test HAVE_BUILTIN_CLZß failed with the following output:\n"
      "${HAVE_BUILTIN_CLZ_COMPILE_OUTPUT}\n" )
    message ( STATUS "  Error compiling helper" )
  else ()
    message ( STATUS "  Wrong result running helper" )
  endif()
endif()

message ( STATUS "Checking for atomics on shm" )
try_run (
  HAVE_ATOMIC_SHM_EXIT HAVE_ATOMIC_SHM_COMPILED
  ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}/config.d/atomics_on_shm.c
  CMAKE_FLAGS -DCOMPILE_DEFINITIONS=-Wall\ -Wconversion\ -Werror -DINCLUDE_DIRECTORIES=${APR1_INCLUDE_DIRS}
  LINK_LIBRARIES PkgConfig::APR1
  COMPILE_OUTPUT_VARIABLE HAVE_ATOMIC_SHM_COMPILE_OUTPUT
)
if ( NOT HAVE_ATOMIC_SHM_COMPILED )
  set( HAVE_ATOMIC_SHM "" CACHE INTERNAL "Test HAVE_ATOMIC_SHM" )
  message ( STATUS "  Not supported" )
  file( APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
    "Performing SOURCE FILE Test HAVE_ATOMIC_SHM failed with the following output:\n"
    "${HAVE_ATOMIC_SHM_COMPILE_OUTPUT}\n" )
elseif ( NOT ${HAVE_ATOMIC_SHM_EXIT} EQUAL 0 )
  set( HAVE_ATOMIC_SHM "" CACHE INTERNAL "Test HAVE_ATOMIC_SHM" )
  message ( STATUS "  Not supported" )
else ()
  set( HAVE_ATOMIC_SHM 1 CACHE INTERNAL "Test HAVE_ATOMIC_SHM" )
  message ( STATUS "  Supported" )
endif()
