# memhook

Small library to hook memory accesses to a page in Linux. Example:

```c
/* cc -o example -std=c11 -D_DEFAULT_SOURCE example.c memhook.c */

#include "memhook.h"
#include <stdio.h>

static volatile size_t PAGE_VALUE = 1234;

size_t read_hook(ptrdiff_t loc) {
    (void)loc;
    return PAGE_VALUE;
}

void write_hook(ptrdiff_t loc, size_t value) {
    (void)loc;
    PAGE_VALUE = value;
}

int main() {
    volatile size_t* page = memhook_setup(read_hook, write_hook);
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
