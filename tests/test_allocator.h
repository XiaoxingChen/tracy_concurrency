#if !defined(__TEST_ALLOCATOR_H__)
#define __TEST_ALLOCATOR_H__

#include <iostream>
#include <vector>
#include <future>
#include <thread>
#include <chrono>
#include <map>

#include "tracy_malloc.h"
#include "tracy_allocator.h"

inline void test_malloc_std()
{
    // Simple malloc test.
    void *ptr = trc::malloc(100);
    if(ptr == nullptr)
    {
        throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__));
    }
    trc::free(ptr);
}

inline void malloc_0()
{
    void* p = trc::malloc(10);
    // if(p == nullptr);
    // {
    //     throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__));
    // }
    trc::free(p);
}

inline void mallocTest01()
{
    size_t heap_size = 0x10000;
    uint8_t heap[heap_size];
    trc::Allocator allocator(heap, heap_size);
    std::vector<void*> pts;
    pts.push_back(allocator.malloc(10));
    // trc::printList();
    pts.push_back(allocator.malloc(12));
    pts.push_back(allocator.malloc(22));
    pts.push_back(allocator.malloc(42));
    pts.push_back(allocator.malloc(62));
    pts.push_back(allocator.malloc(2));

    // trc::printList();

    while(pts.size() > 0)
    {
        allocator.free(pts.back());
        pts.pop_back();
    }
    // std::cout << "===final===" << std::endl;

    if(!allocator.isCompletelyFree())
    {
        // trc::printList();
        throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__));
    }

}

inline void mallocTest02()
{
    size_t heap_size = 0x10000;
    uint8_t heap[heap_size];
    trc::Allocator allocator(heap, heap_size);

    std::vector<void*> pts;
    for(size_t i = 0; i < 5; i++)
    {
        pts.push_back(allocator.malloc( rand() % 4096 ));
    }
    while(pts.size() > 0)
    {
        allocator.free(pts.back());
        pts.pop_back();
    }
    if(!allocator.isCompletelyFree())
    {
        throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__));
    }
    allocator.heapCheck();
}

inline void mallocTest03()
{
    size_t heap_size = 0x80000;
    uint8_t heap[heap_size];
    trc::Allocator allocator(heap, heap_size);

    std::cout << "\nmallocTest03()" << std::endl;
    size_t allocate_num = 55;
    std::vector<std::promise<void*>> ptr_promises(allocate_num);
    std::vector<std::future<void*>> ptr_futures(allocate_num);

    for(size_t i = 0; i < allocate_num; i++)
    {
        ptr_futures.at(i) = ptr_promises.at(i).get_future();
        std::thread([&ptr_promises, &allocator, i]{
            auto ptr = allocator.malloc(rand() % 4096);
            // p.set_value_at_thread_exit(nullptr);
            ptr_promises.at(i).set_value_at_thread_exit(ptr);
            // std::cout << "i: " << i << std::endl;
        }).detach();
    }

    // std::this_thread::sleep_for(std::chrono::seconds(2));
    // std::cout << "fuck" << std::endl;

    for(size_t i = 0; i < allocate_num; i++)
    {
        ptr_futures.at(i).wait();
        // allocator.free(ptr_futures.at(i).get());
    }

    allocator.heapCheck();
    // allocator.printList();

    for(size_t i = 0; i < allocate_num; i++)
    {
        // ptr_futures.at(i).wait();
        void* addr = ptr_futures.at(i).get();
        allocator.free(addr);
        // std::cout << std::hex << "release: " << addr << std::endl;
        allocator.heapCheck();
    }

    if(!allocator.isCompletelyFree())
    {
        allocator.heapCheck();
        allocator.printList();
        throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__));
    }

}

inline void mallocTest04()
{
    std::cout << "\nmallocTest04()" << std::endl;
    size_t heap_size = 0x80000;
    uint8_t heap[heap_size];
    trc::Allocator allocator(heap, heap_size);
    auto func = [&allocator](){
        std::vector<void*> ptrs;
        for(size_t i = 0; i < 500; i++)
        {
            bool do_alloc = rand() % 2;
            if(ptrs.empty() < 5) do_alloc = true;
            if(do_alloc)
            {
                ptrs.push_back(allocator.malloc(rand() % 4096));
            }else // do free
            {
                size_t idx = rand() % ptrs.size();
                allocator.free( ptrs.at(idx) );
                ptrs.at(idx) = ptrs.back();
                ptrs.pop_back();
            }
        }
        while (!ptrs.empty())
        {
            allocator.free(ptrs.back());
            ptrs.pop_back();
        }
    };

    auto t_start = std::chrono::system_clock::now();
    std::vector<std::thread> threads;
    for(size_t i = 0;i < 30; i++) threads.push_back(std::thread(func));
    for(auto & th: threads) th.join();

    auto t_end = std::chrono::system_clock::now();
    std::cout << "t_cost(s): " << std::chrono::duration<double>(t_end - t_start).count() << std::endl;

    if(allocator.heapCheck() != 0)
    {
        allocator.printList();
        throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__));
    }
}

inline void setExitCallBack()
{

}

inline void threadExitCallbackTest01()
{
    auto func = [](){ setExitCallBack(); };
    std::vector<std::thread> threads;
    for(size_t i = 0; i < 5; i++) threads.push_back(std::thread(func));
    // std::cout << "thread num: " << threads.size() << std::endl;
    for(auto & th: threads) th.join();
}

inline void allocatorFullTests()
{
    test_malloc_std();
    malloc_0();
    mallocTest01();
    mallocTest02();
    mallocTest03();
    mallocTest04();
    threadExitCallbackTest01();
}

#endif // __TEST_ALLOCATOR_H__
