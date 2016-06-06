cmake_minimum_required(VERSION 3.10)
project(mitosha)

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -fPIC")
set(CMAKE_C_FLAGS    "${CMAKE_C_FLAGS} -std=c11 -Wall -fPIC")

set(PRJ_FLAGS_DEBUG  "-O0 -g -D_DEBUG -DMUTEST -D_GNU_SOURCE")
set(PRJ_FLAGS_RELEASE "-O2 -DNDEBUG -DMUTEST -D_GNU_SOURCE")

set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS} ${PRJ_FLAGS_DEBUG}")
set(CMAKE_C_FLAGS_DEBUG   "${CMAKE_C_FLAGS} ${PRJ_FLAGS_DEBUG}")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} ${PRJ_FLAGS_RELEASE}")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} ${PRJ_FLAGS_RELEASE}")

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fPIC")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

link_directories(
    ${CMAKE_SOURCE_DIR}/lib
)

include_directories(
    ${CMAKE_SOURCE_DIR}/include    
)

add_subdirectory(src)

if (WITHTEST)

enable_testing()

if(CMAKE_CONFIGURATION_TYPES)
    add_custom_target(tests COMMAND ${CMAKE_CTEST_COMMAND}
        --force-new-ctest-process --output-on-failure
        --build-config "$<CONFIGURATION>")
else()
    add_custom_target(tests COMMAND ${CMAKE_CTEST_COMMAND}
        --force-new-ctest-process --output-on-failure)
endif()

include(../mutest/add_mutest.cmake RESULT_VARIABLE MUTEST)

add_subdirectory(tests)

endif()

