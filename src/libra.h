#ifndef LIBRA_H
#define LIBRA_H
/* Timing and operation counters. */
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
    long long comparisons;       /* Selected comparisons or algorithm steps. */
    long long multiplications;   /* Multiplications instrumented in the math examples. */
    long long aux_bytes;         /* Largest explicitly modeled workspace. */
} Counters;
static inline void counters_reset(Counters *c) {
    if (c) {
        c->comparisons = 0;
        c->multiplications = 0;
        c->aux_bytes = 0;
    }
}
/* Peak modeled algorithm workspace;
does not include input/output or allocator overhead. */
static inline void counters_memory(Counters *c, size_t bytes) {
    if (c && (long long)bytes > c->aux_bytes) c->aux_bytes = (long long)bytes;
}
/* Wall-time measurement for the benchmark protocol (monotonic clock) */
typedef struct {
#ifdef _WIN32
    LARGE_INTEGER start;
    LARGE_INTEGER freq;
#else
    struct timespec start;
#endif
} Timer;
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

/* Search, sorting and LCS interfaces. */
#include <stddef.h>
/* C functions: arrays + lengths + explicit output buffers. */
typedef enum {
    NAIVE, KMP, RABIN_KARP, AUTOMATON
} Matcher;
const char *matcher_name(Matcher engine);
size_t match_text(const char *text, const char *pattern, Matcher engine, size_t *positions, size_t capacity, Counters *stats);
/* A comparator defines the order;
negative/zero/positive means before/equal/after.    The sorting/searching algorithm itself is implemented only once. */
typedef int (*Compare)(const void *a, const void *b);
int compare_ints(const void *a, const void *b);
int binary_iter(const void *array, int n, size_t width, const void *key, Compare compare, Counters *stats);
int binary_rec(const void *array, int n, size_t width, const void *key, Compare compare, Counters *stats);
int merge_sort(void *array, int n, size_t width, Compare compare, Counters *stats);
int quick_sort(void *array, int n, size_t width, Compare compare, Counters *stats);
int lcs(const char *a, const char *b, char *output, size_t capacity, Counters *stats);

/* Remaining required algorithms. */
// Full DAA Syllabus Workbench in pure C.
// PBL handbook lines 6-39 (Common Syllabus Implementation Standard).
#include <stddef.h>
#include <stdbool.h>
// 1. Divide and Conquer
int libraa_max_subarray_dc(const int *arr, int low, int high, int *out_start, int *out_end, Counters *c);
long long libraa_fast_pow_mod(long long base, long long exp, long long mod, Counters *c);
int libraa_celebrity_find(const int *matrix, int n, Counters *c);
void libraa_matrix_multiply_classical(int n, const double *A, const double *B, double *C, Counters *c);
void libraa_matrix_multiply_strassen(int n, const double *A, const double *B, double *C, Counters *c);
// 2. Greedy Algorithms
typedef struct {
    int id;
    int start;
    int finish;
} Activity;
int libraa_activity_selection(Activity *activities, int n, int *out_selected, Counters *c);
int libraa_interval_partitioning(const Activity *intervals, int n, int *out_room_assigned, Counters *c);
typedef struct {
    int left[511], right[511], symbol[511], root;
} HuffmanTree;
int libraa_huffman_encode(const char *text, char *encoded, size_t capacity, HuffmanTree* tree, Counters *c);
int libraa_huffman_decode(const HuffmanTree* tree, const char *encoded, char *decoded, size_t capacity);
void libraa_dijkstra(int n, int src, const double *adj, double *out_dist, int *out_parent, Counters *c);
double libraa_prim_mst(int n, const double *adj, int *out_parent, Counters *c);
double libraa_kruskal_mst(int n, int edge_count, const int *u, const int *v, const double *w, Counters *c);
// 3. Dynamic Programming
int libraa_knapsack_01_dp(int n, int W, const int *weights, const int *values, int *out_selected, Counters *c);
int libraa_matrix_chain_order(const int *p, int n, Counters *c);
double libraa_optimal_bst_roots(const double *p, const double *q, int n, int *roots, Counters *c);
bool libraa_bellman_ford(int n, int m, const int *u, const int *v, const double *w, int src, double *out_dist, Counters *c);
void libraa_floyd_warshall(int n, const double *adj, double *out_dist, Counters *c);
double libraa_tsp_dp(int n, const double *dist_matrix, int *out_path, Counters *c);
// 4. Backtracking & Branch and Bound
int libraa_nqueens(int n, int *out_first_solution, Counters *c);
int libraa_sum_of_subsets(const int *set, int n, int target_sum, int *out_solution, Counters *c);
typedef enum {
    BB_FIFO, BB_LIFO, BB_BEST_BOUND
} BBOrder;
int libraa_knapsack_01_bb(int n, int W, const int *weights, const int *values, int *selected, BBOrder order, Counters *stats);
// 5. Flow and Matching
int libraa_ford_fulkerson(int n, const int *cap_matrix, int src, int sink, Counters *c);
int libraa_max_bipartite_matching(int left_count, int right_count, const int *adj, int *out_match, Counters *c);
// 6. Number Theory
long long libraa_chinese_remainder_theorem(const long long *num, const long long *rem, int k, Counters *c);

/* Library records and product interfaces. */
#include <stdio.h>
/* LIVE EDIT: change one setting, rebuild, then predict and check the result. */
#define TITLE_WEIGHT 3.0
#define KEYWORD_WEIGHT 2.0
#define DESCRIPTION_WEIGHT 1.0
#define RECOMMENDATION_LIMIT 5
#define KEYWORD_SIMILARITY_WEIGHT 0.5
#define SUBJECT_SIMILARITY_WEIGHT 0.3
#define TITLE_SIMILARITY_WEIGHT 0.2
#define MAX_BOOKS 10000
#define MAX_KEYWORDS 32
/* One row from data/books.csv. */
typedef struct {
    int id, copies, hours, value;
    char title[257], author[129], subject[129], description[2049];
    char keywords[MAX_KEYWORDS][65];
    int keyword_count;
} Book;
typedef struct {
    char word[65];
    int book_id;
} IndexEntry;
typedef struct {
    int id;
    double score;
} Hit;
typedef struct {
    int id;
    char name[64];
    int history[8], history_count;
} Reader;
typedef struct {
    int from, to;
} Citation;
typedef struct {
    Book *books;
    int count;
    IndexEntry *index;
    int index_count;
    Reader readers[2];
    int reader_count;
    Citation citations[1];
    int citation_count;
} Library;
void library_init(Library *lib);
void library_free(Library *lib);
int library_load(Library *lib, const char *path, char *error, size_t error_capacity);
const Book *find_book(const Library *lib, int id, int recursive);
int keyword_lookup(const Library *lib, const char *word, int *ids, int capacity);
int search_books(const Library *lib, const char *query, Matcher engine, Hit *hits, int capacity, Counters *stats);
int recommend_books(const Library *lib, int id, Hit *hits, int capacity);
int export_reports(const Library *lib, const char *folder);
void normalize(char *text);
int make_directory(const char *path);
int run_lab(FILE *out);
int run_scenarios(const Library *lib, FILE *out);
int run_benchmarks(FILE *out);
int run_tests(FILE *out);
int run_complete_comparisons(const Library *lib, FILE *out);
#endif
