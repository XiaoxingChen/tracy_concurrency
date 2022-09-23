cmake_minimum_required(VERSION 3.2)

if(CMAKE_SYSTEM_NAME MATCHES "^Android")
    set(THREAD_LIB stdc++)
elseif(CMAKE_SYSTEM_NAME MATCHES "^Linux")
    set(THREAD_LIB pthread)
endif()

add_executable(test_main test_main.cpp)
target_include_directories(test_main PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../)
target_link_libraries(test_main PUBLIC tracy_malloc ${THREAD_LIB} spdlog)
