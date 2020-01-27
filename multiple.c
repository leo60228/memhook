/* cc -o multiple -std=c11 -D_DEFAULT_SOURCE multiple.c memhook.c */

#include "memhook.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static size_t PAGE_VALUE = 1234;

void read_hook(char* loc, ucontext_t* ctx) {
    (void)ctx;
    memcpy(loc, &PAGE_VALUE, sizeof(size_t));
}

void write_hook_a(const char* loc, ucontext_t* ctx) {
    (void)ctx;
    memcpy(&PAGE_VALUE, loc, sizeof(size_t));
}

void write_hook_b(const char* loc, ucontext_t* ctx) {
    (void)loc;
    (void)ctx;
}

int main() {
    long page_size = sysconf(_SC_PAGESIZE);

    volatile size_t* page_a = memhook_setup(NULL, page_size, read_hook, write_hook_a);
    printf("memhook_setup() == %p\n", page_a);
    if (!page_a) {
        return 1;
    }

    volatile size_t* raw_page_b = aligned_alloc(page_size, page_size);
    volatile size_t* page_b = memhook_setup((void*) raw_page_b, page_size, read_hook, write_hook_b);
    printf("memhook_setup() == %p\n", page_b);
    if (!page_b) {
        return 1;
    }

    printf("page_a[10] == %zu\n", page_a[10]);
    printf("page_a[20] = 5678;\n");
    page_a[20] = 5678;
    printf("page_a[30] == %zu\n", page_a[30]);

    printf("page_b[10] == %zu\n", page_b[10]);
    printf("page_b[20] = 1234;\n");
    page_b[20] = 1234;
    printf("page_b[30] == %zu\n", page_b[30]);
}
