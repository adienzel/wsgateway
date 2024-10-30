#FinfScylladbCppDriver.cmake
find_path(SCYLLA_INCLUDE_DIR NAMES cassandra.h)
find_library(SCYLLA_LIBRARY NAMES scylla-cpp-driver)

message(STATUS "SCYLLA_LIBRERIY = ${SCYLLA_LIBRARY}")

if (SCYLLA_INCLUDE_DIR AND SCYLLA_LIBRARY)
    set(SCYLLA_FOUND TRUE)
    set(SCYLLA_LIBRERIES ${SCYLLA_LIBRARY})
    set(SCYLLA_INCLUDE_DIRS ${SCYLLA_INCLUDE_DIR})
else()
    set(SCYLLA_FOUND FALSE)
endif()

mark_as_advanced(SCYLLA_INCLUDE_DIR SCYLLA_LIBRARY)
    