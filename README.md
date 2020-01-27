# memhook

Small library to hook memory accesses to a page in Linux. Example:

```c
/* cc -o example -std=c11 -D_DEFAULT_SOURCE example.c memhook.c */

#include "memhook.h"
#include <stdio.h>
#include <string.h>

static size_t PAGE_VALUE = 1234;

void read_hook(char* loc, ucontext_t* ctx) {
    (void)ctx;
    memcpy(loc, &PAGE_VALUE, sizeof(size_t));
}

void write_hook(const char* loc, ucontext_t* ctx) {
    (void)ctx;
    memcpy(&PAGE_VALUE, loc, sizeof(size_t));
}

int main() {
    volatile size_t* page = memhook_setup(NULL, 4096, read_hook, write_hook);
    printf("memhook_setup() == %p\n", page);
    if (!page) {
        return 1;
    }

    printf("page[10] == %zu\n", page[10]); // page[10] == 1234
    printf("page[20] = 5678;\n");
    page[20] = 5678;
    printf("page[30] == %zu\n", page[30]); // page[30] == 5678
}
```
