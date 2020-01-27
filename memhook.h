#ifndef MEMHOOK_H
#define MEMHOOK_H
#include <stdint.h>
#include <stddef.h>

typedef size_t (*memhook_read_hook)(ptrdiff_t);
typedef void (*memhook_write_hook)(ptrdiff_t, size_t);

void* memhook_setup(memhook_read_hook read, memhook_write_hook write);
#endif
