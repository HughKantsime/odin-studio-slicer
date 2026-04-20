// tbbmalloc_shim.c — weak shims forwarding TBB scalable allocator symbols to
// system malloc. libslic3r objects reference these because the upstream TBB
// build ships tbbmalloc as a dylib we don't want to embed on iOS. Perf impact:
// falls back to libc allocator under concurrent mesh ops — acceptable for v1.

#include <stdlib.h>
#include <string.h>
#include <errno.h>

void *scalable_malloc(size_t size) { return malloc(size); }
void  scalable_free(void *ptr)     { free(ptr); }
void *scalable_realloc(void *ptr, size_t size) { return realloc(ptr, size); }
void *scalable_calloc(size_t n, size_t size)   { return calloc(n, size); }

void *scalable_aligned_malloc(size_t size, size_t alignment) {
    void *p = NULL;
    if (alignment < sizeof(void *)) alignment = sizeof(void *);
    if (posix_memalign(&p, alignment, size) != 0) return NULL;
    return p;
}
void scalable_aligned_free(void *ptr) { free(ptr); }
void *scalable_aligned_realloc(void *ptr, size_t size, size_t alignment) {
    (void)alignment;
    return realloc(ptr, size);
}

size_t scalable_msize(void *ptr) { (void)ptr; return 0; }

int scalable_allocation_mode(int mode, intptr_t value) {
    (void)mode; (void)value; return 0;
}
int scalable_allocation_command(int cmd, void *param) {
    (void)cmd; (void)param; return 0;
}

// __TBB_malloc_safer_* variants used when tbbmalloc is proxying libc.
void *__TBB_malloc_safer_malloc(size_t size, void *(*original)(size_t)) {
    (void)original; return malloc(size);
}
void  __TBB_malloc_safer_free(void *ptr, void (*original)(void *)) {
    (void)original; free(ptr);
}
void *__TBB_malloc_safer_realloc(void *ptr, size_t size, void *(*original)(void *, size_t)) {
    (void)original; return realloc(ptr, size);
}
