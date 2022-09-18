#include <iostream>
#include "test_allocator.h"
#include "test_concurrent_server.h"



int main(int argc, char const *argv[])
{
    std::cout << "start" << std::endl;
    allocatorFullTests();
    concurrentServerFullTests();
    return 0;
}
