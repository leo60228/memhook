/* cc -o bench -std=c11 -D_DEFAULT_SOURCE bench.c memhook.c */

#include "memhook.h"
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char* FAKE_PAGE = NULL;
static char PAGE[4096];
static volatile sig_atomic_t ALARM = 0;

void alarm_handler(int sig) {
    (void)sig;
    ALARM = 1;
}

void read_hook(char* loc) {
    *loc = PAGE[loc - FAKE_PAGE];
}

void write_hook(const char* loc) {
    PAGE[loc - FAKE_PAGE] = *loc;
}

int main() {
    char* fake_page = memhook_setup(NULL, 4096, read_hook, write_hook);
    printf("memhook_setup() == %p\n", fake_page);
    if (!fake_page) {
        return 1;
    }
    FAKE_PAGE = fake_page;

    char* real_page = malloc(4096);

    char* random_data = malloc(4096);
    FILE* urandom = fopen("/dev/urandom", "rb");
    fread(random_data, 4096, 1, urandom);
    fclose(urandom);

    signal(SIGALRM, alarm_handler);

    unsigned long long iterations = 0;

    alarm(1);

    while (!ALARM) {
        memcpy(real_page, random_data, 4096);
        __asm__ volatile("" : : : "memory");
        ++iterations;
    }

    printf("real memory: %llu copies of 4096 bytes in 1 second\n", iterations);

    iterations = 0;
    ALARM = 0;

    alarm(1);

    while (!ALARM) {
        memcpy(fake_page, random_data, 4096);
        __asm__ volatile("" : : : "memory");
        ++iterations;
    }

    printf("intercepted memory: %llu copies of 4096 bytes in 1 second\n", iterations);
}
