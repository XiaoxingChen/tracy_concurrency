#include <iostream>
#include "tracy_malloc.h"

void test_malloc_std()
{
    // Simple malloc test.
    void *ptr = trc::malloc(100);
    if(ptr == nullptr)
    {
        throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__));
    }
    free(ptr);
}

void malloc_0() {
    void* p = trc::malloc(0);
    // if(p == nullptr);
    // {
    //     throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__));
    // }
    free(p);
}

int main(int argc, char const *argv[])
{
    std::cout << "start" << std::endl;
    test_malloc_std();
    malloc_0();
    return 0;
}
