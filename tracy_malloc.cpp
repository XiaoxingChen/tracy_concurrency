#include <cstdlib>
#include <cmath>
#include "tracy_malloc.h"

#define ALLOW_POTENTIAL_MALLOC 1

#if ALLOW_POTENTIAL_MALLOC
#include <string>
#include <iostream>
#include <sstream>

#endif

namespace trc
{
constexpr size_t heapSize() { return 0x100000; }
void* heapStart()
{
    static unsigned char static_heap[heapSize()];
    return static_cast<void*>(static_heap);
}
struct BlockHeader
{
    bool is_free = false;
    size_t block_size = 0;
    BlockHeader* next = nullptr;
    BlockHeader* prev = nullptr;

    void* memAddr() { return reinterpret_cast<void*>((unsigned char*)this + sizeof(BlockHeader)); }
    BlockHeader* nextAdj()
    {
        auto next_start_addr = (unsigned char*)this + block_size;
        if(next_start_addr > (reinterpret_cast<unsigned char*>(heapStart()) + heapSize()))
            return nullptr;
        return reinterpret_cast<BlockHeader*>(next_start_addr);
    }
};

#if ALLOW_POTENTIAL_MALLOC
std::string to_string(const BlockHeader& block)
{
    std::string ret;
    std::ostringstream ss;
    ret += "head addr: ";
    ss << std::hex << &block;
    ret += ss.str();
    ret += ", is_free: ";
    ret += std::to_string(block.is_free);
    ret += ", block_size: ";
    ret += std::to_string(block.block_size);
    return ret;
}
#endif



class BlockLinkedList
{
public:
    BlockLinkedList(bool is_free):
        is_free_(is_free),
        dummy_head_{is_free, 0, nullptr, nullptr},
        tail_(&dummy_head_)
    {
        if(is_free_ && dummy_head_.next == nullptr)
        {
            auto free_block = reinterpret_cast<BlockHeader*>(heapStart());
            free_block->block_size = heapSize();
            push_back(free_block);
        }
        while(tail_->next) tail_ = tail_->next;
    }
    void erase(const BlockHeader& block)
    {
        if(block.prev) block.prev->next = block.next;
        if(block.next) block.next->prev = block.prev;
    }
    void push_back(BlockHeader* block)
    {
        if(block == nullptr) return;
        block->is_free = is_free_;
        tail_->next = block;
        block->prev = tail_;
        tail_ = tail_->next;
    }
    BlockHeader* head() { return &dummy_head_; }
    BlockHeader* tail() { return tail_; }

private:
    bool is_free_;
    BlockHeader dummy_head_;
    BlockHeader* tail_;
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

BlockHeader* findFreeBlock(size_t size)
{
    BlockHeader* p = freeLinkedList().head();
    while(p && p->block_size < size)
    {
        p = p->next;
    }

    return p;
}

BlockHeader* findPrevAdj(BlockHeader* target)
{
    BlockHeader* p = freeLinkedList().head();
    while (p)
    {
        if(p->nextAdj() == target) return p;
        p = p->next;
    }

    p = allocatedLinkedList().head();
    while (p)
    {
        if(p->nextAdj() == target) return p;
        p = p->next;
    }
    return nullptr;
}


size_t memorySizeNeeded(size_t malloc_size)
{
    size_t sum_size = malloc_size + sizeof(BlockHeader);
    size_t ret = std::pow(2, std::ceil(std::log(sum_size)/std::log(2)));
    return ret;
}

void mergeAdjacentFreeBlocks(BlockHeader* header)
{
    if(! header || !(header->is_free)) return;
    while(header->nextAdj() && header->nextAdj()->is_free)
    {
        auto store_next = header->nextAdj();
        header->block_size += header->nextAdj()->block_size;
        freeLinkedList().erase(*store_next);
    }
}

void *malloc(size_t size)
{
    size_t block_size_needed = memorySizeNeeded(size);
    BlockHeader* free_block = findFreeBlock(block_size_needed);

    if(free_block == nullptr) return nullptr;

    if(block_size_needed == free_block->block_size)
    {
        freeLinkedList().erase(*free_block);
        allocatedLinkedList().push_back(free_block);
    }else // block_size_needed < free_block->block_size
    {
        free_block->block_size -= block_size_needed;
        auto new_block = reinterpret_cast<BlockHeader*>( (unsigned char*)free_block + free_block->block_size);
        new_block->block_size = block_size_needed;
        allocatedLinkedList().push_back(new_block);
    }
    return allocatedLinkedList().tail()->memAddr();
}

void free(void *ptr)
{
    BlockHeader* header = reinterpret_cast<BlockHeader*>((unsigned char*)(ptr) - sizeof(BlockHeader));
    auto prev_adj = findPrevAdj(header);
    allocatedLinkedList().erase(*header);
    freeLinkedList().push_back(header);
    mergeAdjacentFreeBlocks(prev_adj);

}

void *calloc(size_t nmemb, size_t size)
{
    return std::calloc(nmemb, size);
}

void *realloc(void *ptr, size_t size)
{
    return std::realloc(ptr, size);
}

void printList()
{

#if ALLOW_POTENTIAL_MALLOC
    BlockHeader* header = freeLinkedList().head();
    std::cout << "===== free =====" << std::endl;
    while(header)
    {
        std::cout << to_string(*header) << std::endl;
        header = header->next;
    }
    std::cout << "===== allocated =====" << std::endl;
    header = allocatedLinkedList().head();
    while(header)
    {
        std::cout << to_string(*header) << std::endl;
        header = header->next;
    }
#endif

}

// size_t malloc_usable_size(void *ptr)
// {
//     return std::malloc_usable_size(ptr);
// }
} // namespace trc
