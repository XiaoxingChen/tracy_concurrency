#include <iostream>
#include "test_allocator.h"
#include "test_concurrent_server.h"
#include "logging.h"



int main(int argc, char const *argv[])
{
    initLogger();
    LOGI("start");
    allocatorFullTests();
    concurrentServerFullTests();
    destroyLogger();
    return 0;
}
