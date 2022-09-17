cmake_minimum_required(VERSION 3.2)
project(tracy_concurrency)

if(CMAKE_SYSTEM_NAME MATCHES "^Android")
    add_compile_options(-std=c++17)
    set(THREAD_LIB stdc++)
elseif(CMAKE_SYSTEM_NAME MATCHES "^Linux")
    add_compile_options(-std=c++17)
    set(THREAD_LIB pthread)
elseif(CMAKE_SYSTEM_NAME MATCHES "^Windows")
    if(MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
    endif(MSVC)
endif()

include_directories(inc)
add_library(tracy_malloc STATIC src/tracy_malloc.cpp)

if(BUILD_TEST)
    add_subdirectory(tests)
endif(BUILD_TEST)

