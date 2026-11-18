#ifndef LIBRAA_WORKBENCH_H
#define LIBRAA_WORKBENCH_H
// Full DAA Syllabus Workbench in pure C.
// PBL handbook lines 6-39 (Common Syllabus Implementation Standard).
#include <stddef.h>
#include <stdbool.h>
#include "counters.h"
// 1. Divide and Conquer
int libraa_max_subarray_dc(const int* arr, int low, int high, int* out_start, int* out_end, Counters* c);
long long libraa_fast_pow_mod(long long base, long long exp, long long mod, Counters* c);
int libraa_celebrity_find(const int* matrix, int n, Counters* c);
void libraa_matrix_multiply_classical(int n, const double* A, const double* B, double* C, Counters* c);
void libraa_matrix_multiply_strassen(int n, const double* A, const double* B, double* C, Counters* c);
// 2. Greedy Algorithms
typedef struct {
    int id;
    int start;
    int finish;
}
Activity;
int libraa_activity_selection(Activity* activities, int n, int* out_selected, Counters* c);
int libraa_interval_partitioning(const Activity* intervals, int n, int* out_room_assigned, Counters* c);
typedef struct {
    int left[511],right[511],symbol[511],root;
}
HuffmanTree;
int libraa_huffman_encode(const char* text,char* encoded,size_t capacity,HuffmanTree* tree,Counters* c);
int libraa_huffman_decode(const HuffmanTree* tree,const char* encoded,char* decoded,size_t capacity);
void libraa_dijkstra(int n, int src, const double* adj, double* out_dist, int* out_parent, Counters* c);
double libraa_prim_mst(int n, const double* adj, int* out_parent, Counters* c);
double libraa_kruskal_mst(int n, int edge_count, const int* u, const int* v, const double* w, Counters* c);
// 3. Dynamic Programming
int libraa_knapsack_01_dp(int n, int W, const int* wt, const int* val, int* out_selected, Counters* c);
int libraa_matrix_chain_order(const int* p, int n, Counters* c);
double libraa_optimal_bst_roots(const double*p,const double*q,int n,int*roots,Counters*c);
bool libraa_bellman_ford(int n, int m, const int* u, const int* v, const double* w, int src, double* out_dist, Counters* c);
void libraa_floyd_warshall(int n, const double* adj, double* out_dist, Counters* c);
double libraa_tsp_dp(int n, const double* dist_matrix, int* out_path, Counters* c);
// 4. Backtracking & Branch and Bound
int libraa_nqueens(int n, int* out_first_solution, Counters* c);
int libraa_sum_of_subsets(const int* set, int n, int target_sum, int* out_solution, Counters* c);
typedef enum {
    BB_FIFO,BB_LIFO,BB_BEST_BOUND
}
BBOrder;
int libraa_knapsack_01_bb(int n,int W,const int *wt,const int *val,int *selected,BBOrder order,Counters *stats);
// 5. Flow and Matching
int libraa_ford_fulkerson(int n, const int* cap_matrix, int src, int sink, Counters* c);
int libraa_max_bipartite_matching(int left_count, int right_count, const int* adj, int* out_match, Counters* c);
// 6. Number Theory
long long libraa_chinese_remainder_theorem(const long long* num, const long long* rem, int k, Counters* c);
#endif /* LIBRAA_WORKBENCH_H */
