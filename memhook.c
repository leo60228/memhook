#include "memhook.h"

#include <ucontext.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>
#include <signal.h>
#include <errno.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __x86_64__
#ifndef REG_EFL
#define REG_EFL 17
#endif
#elif defined(__i386)
#ifndef REG_EFL
#define REG_EFL 16
#endif
#endif

typedef struct memhook_t {
    void* page;
    size_t size;
    memhook_read_hook read_hook;
    memhook_write_hook write_hook;
    _Atomic(struct memhook_t*) next;
} memhook_t;

static _Atomic(memhook_t*) HOOKS = NULL;
static struct sigaction OLD_SIGSEGV;
static _Thread_local size_t* FAULT_ADDR = NULL;
static _Thread_local memhook_t* FAULT_HOOK = NULL;
static struct sigaction OLD_SIGTRAP;
static atomic_flag HANDLERS_SET = ATOMIC_FLAG_INIT;

static void sigsegv_handler(int signal, siginfo_t* info, void* platform) {
    (void)signal;
    memhook_t* hook = atomic_load(&HOOKS);

    do {
        if (info->si_addr >= hook->page && info->si_addr < hook->page + hook->size) {
            ucontext_t* context = platform;
            mcontext_t* mcontext = &context->uc_mcontext;
            mprotect(hook->page, hook->size, PROT_READ | PROT_WRITE);
            FAULT_ADDR = info->si_addr;
            FAULT_HOOK = hook;
            hook->read_hook((char*) FAULT_ADDR, context);
            mcontext->gregs[REG_EFL] |= 0x100;
            return;
        }
        hook = atomic_load(&hook->next);
    } while (hook);

    sigaction(SIGSEGV, &OLD_SIGSEGV, NULL);
}

static void sigtrap_handler(int signal, siginfo_t* info, void* platform) {
    (void)signal;
    (void)info;

    if (FAULT_ADDR) {
        ucontext_t* context = platform;
        mcontext_t* mcontext = &context->uc_mcontext;
        FAULT_HOOK->write_hook((const char*) FAULT_ADDR, context);
        mprotect(FAULT_HOOK->page, FAULT_HOOK->size, PROT_NONE);
        mcontext->gregs[REG_EFL] &= ~0x100;
        FAULT_ADDR = NULL;
        return;
    }

    sigaction(SIGSEGV, &OLD_SIGSEGV, NULL);
}

static void memhook_push(memhook_t* hook) {
    _Atomic(memhook_t*)* tail = &HOOKS;
    memhook_t* expected = NULL;
    while (!atomic_compare_exchange_weak(tail, &expected, hook)) {
        if (expected) {
            tail = &expected->next;
            expected = NULL;
        }
    }
}

void* memhook_setup(void* addr, size_t size, memhook_read_hook read, memhook_write_hook write) {
    memhook_t* hook = malloc(sizeof(memhook_t));
    hook->read_hook = read;
    hook->write_hook = write;

    long page_size = sysconf(_SC_PAGESIZE);
    size_t rounded = (size + page_size - 1) & ~(page_size - 1);
    if (!addr) {
        addr = mmap(NULL, rounded, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (!addr) return NULL;
    } else {
        mprotect(addr, rounded, PROT_NONE);
    }
    hook->page = addr;
    hook->size = rounded;

    memhook_push(hook);

    if (!atomic_flag_test_and_set(&HANDLERS_SET)) {
        struct sigaction sigsegv_action;
        sigsegv_action.sa_sigaction = sigsegv_handler;
        sigsegv_action.sa_flags = SA_SIGINFO;
        if (sigemptyset(&sigsegv_action.sa_mask)) return NULL;
        if (sigaction(SIGSEGV, &sigsegv_action, &OLD_SIGSEGV)) return NULL;

        struct sigaction sigtrap_action;
        sigtrap_action.sa_sigaction = sigtrap_handler;
        sigtrap_action.sa_flags = SA_SIGINFO;
        if (sigemptyset(&sigtrap_action.sa_mask)) return NULL;
        if (sigaction(SIGTRAP, &sigtrap_action, &OLD_SIGTRAP)) return NULL;
    }

    return hook->page;
}
