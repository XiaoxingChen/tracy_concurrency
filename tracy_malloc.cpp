#include <cstdlib>
#include <cmath>
#include "tracy_malloc.h"

namespace trc
{
struct Header
{
    bool is_free = false;
    size_t block_size = 0;
    Header* next = nullptr;
    Header* prev = nullptr;

    void* memAddr() { return reinterpret_cast<void*>((unsigned char*)this + sizeof(Header)); }
};

constexpr size_t heapSize() { return 0x100000; }
void* heapStart()
{
    static unsigned char static_heap[heapSize()];
    return static_cast<void*>(static_heap);
}

class BlockLinkedList
{
public:
    BlockLinkedList(bool is_free):
        is_free_(is_free),
        dummy_head_{is_free, 0, nullptr, nullptr}
    {
        if(is_free_ && dummy_head_.next == nullptr)
        {
            auto free_block = reinterpret_cast<Header*>(heapStart());
            free_block->block_size = heapSize();
            push_back(free_block);
        }
        tail_ = &dummy_head_;
        while(tail_->next) tail_ = tail_->next;
    }
    void erase(const Header& block)
    {
        block.prev->next = block.next;
        block.next->prev = block.prev;
    }
    void push_back(Header* block)
    {
        if(block == nullptr) return;
        block->is_free = is_free_;
        tail_->next = block;
        block->prev = tail_;
        tail_ = tail_->next;
    }
    Header* head() { return &dummy_head_; }
    Header* tail() { return tail_; }

private:
    bool is_free_;
    Header dummy_head_;
    Header* tail_;
};

BlockLinkedList& allocatedLinkedList()
{
    static BlockLinkedList list(false);
    return list;
}

BlockLinkedList& freeLinkedList()
{
    static BlockLinkedList list(true);
    return list;
}

Header* findFreeBlock(size_t size)
{
    Header* p = freeLinkedList().head();
    while(p && p->block_size < size)
    {
        p = p->next;
    }

    return p;
}


size_t memorySizeNeeded(size_t malloc_size)
{
    size_t sum_size = malloc_size + sizeof(Header);
    size_t ret = std::pow(2, std::ceil(std::log(sum_size)/std::log(2)));
    return ret;
}

void *malloc(size_t size)
{
    size_t block_size_needed = memorySizeNeeded(size);
    Header* free_block = findFreeBlock(block_size_needed);

    if(free_block == nullptr) return nullptr;

    if(block_size_needed == free_block->block_size)
    {
        freeLinkedList().erase(*free_block);
        allocatedLinkedList().push_back(free_block);
    }else // block_size_needed < free_block->block_size
    {
        free_block->block_size -= block_size_needed;
        auto new_block = reinterpret_cast<Header*>( (unsigned char*)free_block + free_block->block_size);
        new_block->block_size = block_size_needed;
        allocatedLinkedList().push_back(new_block);
    }
    return allocatedLinkedList().tail()->memAddr();
}

void free(void *ptr)
{
    std::free(ptr);
    return;
}

void *calloc(size_t nmemb, size_t size)
{
    return std::calloc(nmemb, size);
}

void *realloc(void *ptr, size_t size)
{
    return std::realloc(ptr, size);
}

// size_t malloc_usable_size(void *ptr)
// {
//     return std::malloc_usable_size(ptr);
// }
} // namespace trc
