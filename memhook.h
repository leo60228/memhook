#ifndef MEMHOOK_H
#define MEMHOOK_H
#include <stdint.h>
#include <stddef.h>
#include <ucontext.h>

typedef void (*memhook_read_hook)(char*, ucontext_t*);
typedef void (*memhook_write_hook)(const char*, ucontext_t*);

void* memhook_setup(void*, size_t, memhook_read_hook read, memhook_write_hook write);
#endif
