#include "memhook.h"

#include <ucontext.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>
#include <signal.h>
#include <errno.h>
#include <stdatomic.h>

#ifdef __x86_64__
#ifndef REG_EFL
#define REG_EFL 17
#endif
#elif defined(__i386)
#ifndef REG_EFL
#define REG_EFL 16
#endif
#endif

static void* PAGE = NULL;
static size_t SIZE = 0;
static struct sigaction OLD_SIGSEGV;
static size_t* FAULT_ADDR = NULL;
static struct sigaction OLD_SIGTRAP;
static memhook_read_hook READ_HOOK = NULL;
static memhook_write_hook WRITE_HOOK = NULL;

static void sigsegv_handler(int signal, siginfo_t* info, void* platform) {
    (void)signal;

    if (info->si_addr >= PAGE && info->si_addr < PAGE + SIZE) {
        ucontext_t* context = platform;
        mcontext_t* mcontext = &context->uc_mcontext;
        mprotect(PAGE, SIZE, PROT_READ | PROT_WRITE);
        FAULT_ADDR = info->si_addr;
        READ_HOOK((char*) FAULT_ADDR, context);
        mcontext->gregs[REG_EFL] |= 0x100;
        return;
    }
    sigaction(SIGSEGV, &OLD_SIGSEGV, NULL);
}

static void sigtrap_handler(int signal, siginfo_t* info, void* platform) {
    (void)signal;
    (void)info;

    if (FAULT_ADDR) {
        ucontext_t* context = platform;
        mcontext_t* mcontext = &context->uc_mcontext;
        WRITE_HOOK((const char*) FAULT_ADDR, context);
        mprotect(PAGE, SIZE, PROT_NONE);
        mcontext->gregs[REG_EFL] &= ~0x100;
        FAULT_ADDR = NULL;
        return;
    }
    sigaction(SIGSEGV, &OLD_SIGSEGV, NULL);
}

void* memhook_setup(void* addr, size_t size, memhook_read_hook read, memhook_write_hook write) {
    READ_HOOK = read;
    WRITE_HOOK = write;

    long page_size = sysconf(_SC_PAGESIZE);
    size_t rounded = (size + page_size - 1) & ~(page_size - 1);
    if (!addr) {
        addr = mmap(NULL, rounded, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (!addr) return NULL;
    } else {
        mprotect(addr, rounded, PROT_NONE);
    }
    PAGE = addr;
    SIZE = rounded;

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

    atomic_signal_fence(memory_order_seq_cst);

    return PAGE;
}
