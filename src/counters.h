#ifndef LIBRAA_COUNTERS_H
#define LIBRAA_COUNTERS_H
// Instrumentation shared by every assessed algorithm in pure C.
// PBL handbook line 11: "collect operation counts, execution time and memory use".
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif
typedef struct {
    long long comparisons;
    /* element/character comparisons */     long long char_ops;
    /* auxiliary character operations (automaton builds...) */     long long hash_ops;
    /* modular multiplications (Rabin-Karp, CRT, fast pow) */     long long assignments;
    /* moves/copies during sorts */     long long aux_bytes;
    /* auxiliary allocations (lps tables, DP rows, queues) */
}
Counters;
static inline void counters_reset(Counters* c) {
    if (c) {
        c->comparisons = 0;
        c->char_ops = 0;
        c->hash_ops = 0;
        c->assignments = 0;
        c->aux_bytes = 0;
    }
}
/* Peak modeled algorithm workspace;
does not include input/output or allocator overhead. */ static inline void counters_memory(Counters *c, size_t bytes) {
    if (c && (long long)bytes > c->aux_bytes) c->aux_bytes = (long long)bytes;
}
/* Wall-time measurement for the benchmark protocol (monotonic clock) */ typedef struct {
#ifdef _WIN32
    LARGE_INTEGER start;
    LARGE_INTEGER freq;
#else
    struct timespec start;
#endif
}
Timer;
static inline void timer_start(Timer* t) {
#ifdef _WIN32
    QueryPerformanceFrequency(&t->freq);
    QueryPerformanceCounter(&t->start);
#else
    clock_gettime(CLOCK_MONOTONIC, &t->start);
#endif
}
static inline long long timer_stop_us(const Timer* t) {
#ifdef _WIN32
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    if (t->freq.QuadPart == 0) return 0;
    return (long long)((now.QuadPart - t->start.QuadPart) * 1000000LL / t->freq.QuadPart);
#else
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (long long)((now.tv_sec - t->start.tv_sec) * 1000000LL +                        (now.tv_nsec - t->start.tv_nsec) / 1000LL);
#endif
}
#endif /* LIBRAA_COUNTERS_H */
