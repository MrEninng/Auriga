#ifndef __ALLOCATION__
#define __ALLOCATION__

void* my_malloc(size_t size);
void my_free(void* ptr);
void* my_realloc(void* ptr, size_t size);
void* my_calloc(size_t size);

#endif //__ALLOCATION__
