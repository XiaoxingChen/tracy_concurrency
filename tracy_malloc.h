#if !defined(__TRACY_MALLOC_H__)
#define __TRACY_MALLOC_H__

namespace trc
{

void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
// size_t malloc_usable_size(void *ptr);

void printList();


} // namespace trc



#endif // __TRACY_MALLOC_H__
