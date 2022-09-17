#if !defined(__TRACY_ALLOCATOR_H__)
#define __TRACY_ALLOCATOR_H__

#include <cstdlib>
#include <stdint.h>
#include <mutex>
#include <functional>

namespace trc
{
struct BlockHeader
{
    bool is_free = false;
    size_t block_size = 0;
    BlockHeader* next = nullptr;
    BlockHeader* prev = nullptr;

    void* memAddr() { return reinterpret_cast<void*>((unsigned char*)this + sizeof(BlockHeader)); }
    BlockHeader* nextAdj();
};

class BlockLinkedList
{
public:
    BlockLinkedList(bool is_free);
    void erase(const BlockHeader& block);
    void push_back(BlockHeader* block);

    void traverse(std::function<void(const BlockHeader& header)> f) const;
    size_t totalBlockSize() const;

    BlockHeader* head() { return &dummy_head_; }
    const BlockHeader* head() const { return &dummy_head_; }
    BlockHeader* tail() { return tail_; }
    const BlockHeader* tail() const { return tail_; }

private:
    const bool is_free_;
    BlockHeader dummy_head_;
    BlockHeader* tail_;
};

class Allocator
{
public:
    Allocator(void* heap_start, size_t heap_size)
        :heap_start_(heap_start), heap_size_(heap_size),
        free_list_(true), allocated_list_(false) {}

    BlockHeader* findFreeBlock(size_t size);
    BlockHeader* findPrevAdj(BlockHeader* target);
    void mergeAdjacentFreeBlocks(BlockHeader* header);
    void *malloc(size_t size);
    void free(void *ptr);
    uint32_t heapCheck() const;
    void printList() const;

    bool isCompletelyFree() const;

private:
    void* heap_start_;
    size_t heap_size_;
    BlockLinkedList free_list_;
    BlockLinkedList allocated_list_;
    std::mutex mtx_;
};

} // namespace trc


#endif // __TRACY_ALLOCATOR_H__
