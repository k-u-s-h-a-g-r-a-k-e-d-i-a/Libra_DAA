#ifndef ALGORITHMS_H
#define ALGORITHMS_H
#include "counters.h"
#include <stddef.h>
/* C functions: arrays + lengths + explicit output buffers. */ typedef enum {
    NAIVE, KMP, RABIN_KARP, AUTOMATON
} Matcher;
const char *matcher_name(Matcher engine);
size_t match_text(const char *text, const char *pattern, Matcher engine,                   size_t *positions, size_t capacity, Counters *stats);
int binary_iter(const int *a, int n, int key, Counters *stats);
int binary_rec(const int *a, int n, int key, Counters *stats);
int merge_ints(int *a, int n, Counters *stats);
void quick_ints(int *a, int n, Counters *stats);
int lcs(const char *a, const char *b, char *output, size_t capacity, Counters *stats);
#endif
