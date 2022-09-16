#include <cstdlib>
#include <cmath>
#include <mutex>
#include <functional>
#include "tracy_allocator.h"

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


BlockHeader* BlockHeader::nextAdj()
{
    auto next_start_addr = (uint8_t*)this + block_size;
    if(next_start_addr < heapStart())
        return nullptr;
    if(next_start_addr >= (uint8_t*)heapStart() + heapSize())
        return nullptr;
    return reinterpret_cast<BlockHeader*>(next_start_addr);
}

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

// std::mutex& heapMutex()
// {
//     static std::mutex mtx;
//     return mtx;
// }


BlockLinkedList::BlockLinkedList(bool is_free):
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

void BlockLinkedList::erase(const BlockHeader& block)
{
    if(&dummy_head_ == &block) return;
    block.prev->next = block.next;
    if(block.next) block.next->prev = block.prev;
    if(tail_ == &block) tail_ = block.prev;
}

void BlockLinkedList::push_back(BlockHeader* block)
{
    if(block == nullptr) return;
    block->is_free = is_free_;
    tail_->next = block;
    block->prev = tail_;
    tail_ = tail_->next;
}

void BlockLinkedList::traverse(std::function<void(const BlockHeader& header)> f) const
{
    auto p = dummy_head_.next;
    while(p)
    {
        f(*p);
        p = p->next;
    }
}

size_t BlockLinkedList::totalBlockSize() const
{
    size_t result = 0;
    traverse([&result](const BlockHeader& b){ result += b.block_size; });
    return result;
}


Allocator& heapAllocator()
{
    static Allocator allocator(heapStart(), heapSize());
    return allocator;
}

#if 0
BlockLinkedList& allocatedLinkedList()
{
    static BlockLinkedList list(false);
    return list;
}

bool isAllocatedListEmpty()
{
    return allocatedLinkedList().tail() == allocatedLinkedList().head();
}


BlockLinkedList& freeLinkedList()
{
    static BlockLinkedList list(true);
    return list;
}
#endif

bool Allocator::isCompletelyFree() const
{
    return allocated_list_.tail() == allocated_list_.head();
}

BlockHeader* Allocator::findFreeBlock(size_t size)
{
    BlockHeader* p = free_list_.head();
    while(p && p->block_size < size)
    {
        p = p->next;
    }

    return p;
}


BlockHeader* Allocator::findPrevAdj(BlockHeader* target)
{
    BlockHeader* p = free_list_.head();
    while (p)
    {
        if(p->nextAdj() == target) return p;
        p = p->next;
    }

    p = allocated_list_.head();
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

void Allocator::mergeAdjacentFreeBlocks(BlockHeader* header)
{
    // std::cout << "in : " << std::hex << header << std::endl;
    if(! header || !(header->is_free)) return;
    while(header->nextAdj() && header->nextAdj()->is_free)
    {
        auto store_next = header->nextAdj();
        // std::cout << "block size: " << store_next->block_size << ", addr: " << std::hex << header << "," << store_next << std::endl;
        header->block_size += header->nextAdj()->block_size;
        free_list_.erase(*store_next);
    }
}

void* Allocator::malloc(size_t size)
{
    std::lock_guard<std::mutex> guard(mtx_);
    size_t block_size_needed = memorySizeNeeded(size);

    BlockHeader* free_block = findFreeBlock(block_size_needed);

    if(free_block == nullptr) return nullptr;

    if(block_size_needed == free_block->block_size)
    {
        free_list_.erase(*free_block);
        allocated_list_.push_back(free_block);
    }else // block_size_needed < free_block->block_size
    {
        free_block->block_size -= block_size_needed;
        auto new_block = reinterpret_cast<BlockHeader*>( (unsigned char*)free_block + free_block->block_size);
        new_block->block_size = block_size_needed;
        allocated_list_.push_back(new_block);
    }
    // std::cout << "hex addr: 0x" << std::hex << allocated_list_.tail()->memAddr() << std::endl;
    return allocated_list_.tail()->memAddr();
}

void *malloc(size_t size)
{
    return heapAllocator().malloc(size);
}

void Allocator::free(void *ptr)
{
    std::lock_guard<std::mutex> guard(mtx_);
    if(nullptr == ptr) return;
    BlockHeader* header = reinterpret_cast<BlockHeader*>((unsigned char*)(ptr) - sizeof(BlockHeader));

    BlockHeader* prev_adj = findPrevAdj(header);
    allocated_list_.erase(*header);
    free_list_.push_back(header);
    // std::cout << "out : " << std::hex << prev_adj << std::endl;
    mergeAdjacentFreeBlocks(prev_adj);
}
void free(void *ptr)
{
    return heapAllocator().free(ptr);
}

void *calloc(size_t nmemb, size_t size)
{
    return std::calloc(nmemb, size);
}

void *realloc(void *ptr, size_t size)
{
    return std::realloc(ptr, size);
}

uint32_t Allocator::heapCheck() const
{
    uint32_t result_mask = 0;
    // size check
    if(free_list_.totalBlockSize() + allocated_list_.totalBlockSize() != heapSize())
    {
        result_mask |= (1 << 0);
#if ALLOW_POTENTIAL_MALLOC
        std::cout << "size check failed! free: " << free_list_.totalBlockSize()
        << ", allocated: " << allocated_list_.totalBlockSize()
        << ", total: " << heapSize() << std::endl;
#endif
    }

    // free flag check
    {
        bool is_all_free = true;
        free_list_.traverse([&is_all_free](const BlockHeader& b){ is_all_free = (is_all_free && b.is_free);});

        if(!is_all_free)
        {
#if ALLOW_POTENTIAL_MALLOC
            std::cout << "free check failed" << std::endl;
#endif
            result_mask |= (1 << 1);
        }

        bool is_any_free = false;
        allocated_list_.traverse([&is_any_free](const BlockHeader& b){ is_any_free = (is_any_free || b.is_free);});
        if(is_any_free)
        {
#if ALLOW_POTENTIAL_MALLOC
            std::cout << "allocated check failed" << std::endl;
#endif
            result_mask |= (1 << 2);
        }

    }
    return result_mask;
}

void Allocator::printList() const
{

#if ALLOW_POTENTIAL_MALLOC
    const BlockHeader* header = free_list_.head();
    std::cout << "===== free =====" << std::endl;
    while(header)
    {
        std::cout << to_string(*header) << std::endl;
        header = header->next;
    }
    std::cout << "===== allocated =====" << std::endl;
    header = allocated_list_.head();
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
