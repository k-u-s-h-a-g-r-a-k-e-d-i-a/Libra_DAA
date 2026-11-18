#ifndef ALGORITHMS_H
#define ALGORITHMS_H
#include "counters.h"
#include <stddef.h>
/* C functions: arrays + lengths + explicit output buffers. */ typedef enum {
    NAIVE, KMP, RABIN_KARP, AUTOMATON
}
Matcher;
const char *matcher_name(Matcher engine);
size_t match_text(const char *text, const char *pattern, Matcher engine,                   size_t *positions, size_t capacity, Counters *stats);
/* A comparator defines the order;
negative/zero/positive means before/equal/after.    The sorting/searching algorithm itself is implemented only once. */ typedef int (*Compare)(const void *a, const void *b);
int compare_ints(const void *a, const void *b);
int binary_iter(const void *array,int n,size_t width,const void *key,Compare compare,Counters *stats);
int binary_rec(const void *array,int n,size_t width,const void *key,Compare compare,Counters *stats);
int merge_sort(void *array,int n,size_t width,Compare compare,Counters *stats);
int quick_sort(void *array,int n,size_t width,Compare compare,Counters *stats);
int lcs(const char *a, const char *b, char *output, size_t capacity, Counters *stats);
#endif
