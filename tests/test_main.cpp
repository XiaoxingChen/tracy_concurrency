#include <iostream>
#include <vector>
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

void mallocTest01()
{
    std::vector<void*> pts;
    pts.push_back(trc::malloc(10));
    // trc::printList();
    pts.push_back(trc::malloc(12));
    pts.push_back(trc::malloc(22));
    pts.push_back(trc::malloc(42));
    pts.push_back(trc::malloc(62));
    pts.push_back(trc::malloc(2));

    trc::printList();

    while(pts.size() > 0)
    {
        trc::free(pts.back());
        pts.pop_back();
    }
    std::cout << "===final===" << std::endl;
    trc::printList();
}

int main(int argc, char const *argv[])
{
    std::cout << "start" << std::endl;
    // test_malloc_std();
    // malloc_0();
    mallocTest01();
    return 0;
}
