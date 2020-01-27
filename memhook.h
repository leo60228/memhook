#ifndef MEMHOOK_H
#define MEMHOOK_H
#include <stdint.h>
#include <stddef.h>

typedef void (*memhook_read_hook)(char*);
typedef void (*memhook_write_hook)(const char*);

void* memhook_setup(void*, size_t, memhook_read_hook read, memhook_write_hook write);
#endif
