/* cc -o example -std=c11 -D_DEFAULT_SOURCE example.c memhook.c */

#include "memhook.h"
#include <stdio.h>
#include <string.h>

static size_t PAGE_VALUE = 1234;

void read_hook(char* loc) {
    memcpy(loc, &PAGE_VALUE, sizeof(size_t));
}

void write_hook(const char* loc) {
    memcpy(&PAGE_VALUE, loc, sizeof(size_t));
}

int main() {
    volatile size_t* page = memhook_setup(NULL, 4096, read_hook, write_hook);
    printf("memhook_setup() == %p\n", page);
    if (!page) {
        return 1;
    }

    printf("page[10] == %zu\n", page[10]);
    printf("page[20] = 5678;\n");
    page[20] = 5678;
    printf("page[30] == %zu\n", page[30]);
}
