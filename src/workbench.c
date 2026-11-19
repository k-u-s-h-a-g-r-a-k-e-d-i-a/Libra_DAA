// src/workbench/workbench.c — Full DAA syllabus implementations in pure C.
// PBL handbook lines 6-39 (Common Syllabus Implementation Standard).
#include "workbench.h"
#include "algorithms.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <limits.h>
// ============================================================================
// 1. DIVIDE AND CONQUER
// ============================================================================
static int max_crossing_sum(const int *arr, int low, int mid, int high, int *out_start, int *out_end, Counters *c) {
    int left_sum = INT_MIN;
    int sum = 0;
    int max_left = mid;
    for (int i = mid; i >= low; --i) {
        sum += arr[i];
        if (c) c->comparisons++;
        if (sum > left_sum) {
            left_sum = sum;
            max_left = i;
        }
    }
    int right_sum = INT_MIN;
    sum = 0;
    int max_right = mid + 1;
    for (int j = mid + 1; j <= high; ++j) {
        sum += arr[j];
        if (c) c->comparisons++;
        if (sum > right_sum) {
            right_sum = sum;
            max_right = j;
        }
    }
    if (out_start) *out_start = max_left;
    if (out_end) *out_end = max_right;
    return left_sum + right_sum;
}

int libraa_max_subarray_dc(const int *arr, int low, int high, int *out_start, int *out_end, Counters *c) {
    if (!arr || low < 0 || high < low) {
        errno = EINVAL;
        return 0;
    }
    if (low == high) {
        if (out_start) *out_start = low;
        if (out_end) *out_end = high;
        return arr[low];
    }
    int mid = low + (high - low) / 2;
    int left_s, left_e;
    int left_sum = libraa_max_subarray_dc(arr, low, mid, &left_s, &left_e, c);
    int right_s, right_e;
    int right_sum = libraa_max_subarray_dc(arr, mid + 1, high, &right_s, &right_e, c);
    int cross_s, cross_e;
    int cross_sum = max_crossing_sum(arr, low, mid, high, &cross_s, &cross_e, c);
    if (c) c->comparisons += 2;
    if (left_sum >= right_sum && left_sum >= cross_sum) {
        if (out_start) *out_start = left_s;
        if (out_end) *out_end = left_e;
        return left_sum;
    }
    else if (right_sum >= left_sum && right_sum >= cross_sum) {
        if (out_start) *out_start = right_s;
        if (out_end) *out_end = right_e;
        return right_sum;
    }
    else {
        if (out_start) *out_start = cross_s;
        if (out_end) *out_end = cross_e;
        return cross_sum;
    }
}

long long libraa_fast_pow_mod(long long base, long long exp, long long mod, Counters *c) {
    long long result;
    if (exp < 0 || mod < 1 || mod > 2000000000LL) {
        errno = EINVAL;
        return -1;
    }
    result = 1%mod;
    base = (base%mod+mod)%mod;
    while (exp) {
        if (exp&1) {
            result = result*base%mod;
            if (c)c->multiplications++;
        }
        base = base*base%mod;
        exp /= 2;
        if (c)c->multiplications++;
    }
    return result;
}

/* Each half sends one possible celebrity. One citation rules out one candidate. */
static int celebrity_candidate(const int *matrix, int n, int first, int last, Counters *stats) {
    if (first == last) return first;
    int middle = first + (last - first) / 2;
    int left = celebrity_candidate(matrix, n, first, middle, stats);
    int right = celebrity_candidate(matrix, n, middle + 1, last, stats);
    if (stats) stats->comparisons++;
    if (matrix[(size_t)left * n + right]) return right;
    return left;
}

int libraa_celebrity_find(const int *matrix, int n, Counters *c) {
    if (!matrix || n <= 0) return -1;
    int candidate = celebrity_candidate(matrix, n, 0, n - 1, c);
    // Verification phase: candidate knows nobody, and everyone knows candidate
    for (int i = 0; i < n; ++i) {
        if (i != candidate) {
            if (c) c->comparisons += 2;
            if (matrix[(size_t)candidate * n + i] || !matrix[(size_t)i * n + candidate]) {
                return -1;
                // No celebrity
            }
        }
    }
    return candidate;
}

void libraa_matrix_multiply_classical(int n, const double *A, const double *B, double *C, Counters *c) {
    if (n < 1 || n > 128 || !A || !B || !C) {
        errno = EINVAL;
        return;
    }
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            double sum = 0.0;
            for (int k = 0; k < n; ++k) {
                sum += A[i * n + k] * B[k * n + j];
                if (c) {
                    c->multiplications++;
                    // multiplications
                    c->comparisons++;
                    // additions
                }
            }
            C[i * n + j] = sum;
        }
    }
}

static void mat_op(int n, const double *a, const double *b, double *out, int sign, Counters *c) {
    for (int i = 0; i < n*n; ++i) {
        out[i] = a[i]+sign*b[i];
        if (c)c->comparisons++;
    }
}

static int strassen_rec(int n, const double *a, const double *b, double *out, Counters *c, size_t parent_bytes) {
    if (n == 1) {
        out[0] = a[0]*b[0];
        if (c)c->multiplications++;
        return 1;
    }
    int h = n/2, z = h*h;
    double *space = calloc((size_t)17*z, sizeof(double));
    if (!space) {
        errno = ENOMEM;
        return 0;
    }
    size_t active_bytes = parent_bytes + 17 * (size_t)z * sizeof(double);
    counters_memory(c, active_bytes);
    double *A[4], *B[4], *M[7], *x = space+15*z, *y = space+16*z;
    for (int q = 0; q < 4; ++q) {
        A[q] = space+q*z;
        B[q] = space+(q+4)*z;
        for (int i = 0; i < h; ++i)for (int j = 0; j < h; ++j) {
            A[q][i*h+j] = a[(i+q/2*h)*n+j+q%2*h];
            B[q][i*h+j] = b[(i+q/2*h)*n+j+q%2*h];
        }
    }
    for (int q = 0; q < 7; ++q)M[q] = space+(8+q)*z;
    int ok = 1;
    mat_op(h, A[0], A[3], x, 1, c);
    mat_op(h, B[0], B[3], y, 1, c);
    ok = ok && strassen_rec(h, x, y, M[0], c, active_bytes);
    mat_op(h, A[2], A[3], x, 1, c);
    ok = ok && strassen_rec(h, x, B[0], M[1], c, active_bytes);
    mat_op(h, B[1], B[3], y, -1, c);
    ok = ok && strassen_rec(h, A[0], y, M[2], c, active_bytes);
    mat_op(h, B[2], B[0], y, -1, c);
    ok = ok && strassen_rec(h, A[3], y, M[3], c, active_bytes);
    mat_op(h, A[0], A[1], x, 1, c);
    ok = ok && strassen_rec(h, x, B[3], M[4], c, active_bytes);
    mat_op(h, A[2], A[0], x, -1, c);
    mat_op(h, B[0], B[1], y, 1, c);
    ok = ok && strassen_rec(h, x, y, M[5], c, active_bytes);
    mat_op(h, A[1], A[3], x, -1, c);
    mat_op(h, B[2], B[3], y, 1, c);
    ok = ok && strassen_rec(h, x, y, M[6], c, active_bytes);
    if (ok)for (int i = 0; i < h; ++i)for (int j = 0; j < h; ++j) {
        int k = i*h+j;
        out[i*n+j] = M[0][k]+M[3][k]-M[4][k]+M[6][k];
        out[i*n+j+h] = M[2][k]+M[4][k];
        out[(i+h)*n+j] = M[1][k]+M[3][k];
        out[(i+h)*n+j+h] = M[0][k]-M[1][k]+M[2][k]+M[5][k];
    }
    free(space);
    return ok;
}

void libraa_matrix_multiply_strassen(int n, const double *A, const double *B, double *C, Counters *c) {
    if (n < 1 || n > 128 || !A || !B || !C) {
        errno = EINVAL;
        return;
    }
    int p = 1;
    while (p < n)p *= 2;
    double *space = calloc((size_t)3*p*p, sizeof(double));
    if (!space) {
        errno = ENOMEM;
        return;
    }
    double *x = space, *y = space+p*p, *out = space+2*p*p;
    for (int i = 0; i < n; ++i)for (int j = 0; j < n; ++j) {
        x[i*p+j] = A[i*n+j];
        y[i*p+j] = B[i*n+j];
    }
    size_t padded_bytes = 3 * (size_t)p * p * sizeof(double);
    counters_memory(c, padded_bytes);
    if (strassen_rec(p, x, y, out, c, padded_bytes))for (int i = 0; i < n; ++i)for (int j = 0; j < n; ++j)C[i*n+j] = out[i*p+j];
    free(space);
}
// ============================================================================
// 2. GREEDY ALGORITHMS
// ============================================================================
static int compare_finish(const void *left, const void *right) {
    const Activity *a = left, *b = right;
    return (a->finish > b->finish)-(a->finish < b->finish);
}

static int compare_start(const void *left, const void *right) {
    const Activity *a = left, *b = right;
    return (a->start > b->start)-(a->start < b->start);
}

int libraa_activity_selection(Activity *activities, int n, int *out_selected, Counters *c) {
    if (!out_selected || n < 0 || n > 256 || (n && !activities)) {
        errno = EINVAL;
        return -1;
    }
    if (!activities || n <= 0) return 0;
    if (!merge_sort(activities, n, sizeof(Activity), compare_finish, c)) return -1;
    for (int i = 0; i < n; ++i)if (activities[i].start < 0 || activities[i].finish <= activities[i].start) {
        errno = EINVAL;
        return -1;
    }
    int count = 0;
    out_selected[count++] = activities[0].id;
    int last_finish = activities[0].finish;
    for (int i = 1; i < n; ++i) {
        if (c) c->comparisons++;
        if (activities[i].start >= last_finish) {
            out_selected[count++] = activities[i].id;
            last_finish = activities[i].finish;
        }
    }
    return count;
}

int libraa_interval_partitioning(const Activity *intervals, int n, int *out_room_assigned, Counters *c) {
    if (n < 0 || n > 256 || (n && !intervals)) {
        errno = EINVAL;
        return -1;
    }
    if (!intervals || n <= 0) return 0;
    for (int i = 0; i < n; ++i)if (intervals[i].start < 0 || intervals[i].finish <= intervals[i].start) {
        errno = EINVAL;
        return -1;
    }
    /* Assign rooms by original array position, even if two events share an ID. */
    Activity sorted[256];
    int count = n < 256 ? n : 256;
    for (int i = 0; i < count; ++i) {
        sorted[i] = intervals[i];
        sorted[i].id = i;
    }
    if (!merge_sort(sorted, count, sizeof(Activity), compare_start, c)) return -1;
    // room_finish[r] stores the finish time of the last activity in room r
    int room_finish[256];
    int num_rooms = 0;
    for (int i = 0; i < count; ++i) {
        int assigned_room = -1;
        for (int r = 0; r < num_rooms; ++r) {
            if (c) c->comparisons++;
            if (room_finish[r] <= sorted[i].start) {
                assigned_room = r;
                break;
            }
        }
        if (assigned_room == -1) {
            assigned_room = num_rooms++;
        }
        room_finish[assigned_room] = sorted[i].finish;
        if (out_room_assigned) {
            out_room_assigned[sorted[i].id] = assigned_room + 1;
        }
    }
    return num_rooms;
}

static void make_codes(const HuffmanTree*t, int node, char *path, int depth, char codes[256][256]) {
    if (t->left[node] < 0) {
        if (depth == 0)path[depth++] = '0';
        path[depth] = '\0';
        strcpy(codes[t->symbol[node]], path);
        return;
    }
    path[depth] = '0';
    make_codes(t, t->left[node], path, depth+1, codes);
    path[depth] = '1';
    make_codes(t, t->right[node], path, depth+1, codes);
}

int libraa_huffman_encode(const char *text, char *out, size_t cap, HuffmanTree*t, Counters *c) {
    int freq[511] = {0}, active[511], count = 0, nodes = 0;
    char codes[256][256] = {
        {0}
    }, path[256];
    size_t bits = 0;
    if (!text || !t || !out || cap == 0) {
        errno = EINVAL;
        return -1;
    }
    out[0] = '\0';
    t->root = -1;
    if (strlen(text) > 2048) {
        errno = EINVAL;
        return -1;
    }
    int symbol_freq[256] = {0};
    for (const unsigned char *p = (const unsigned char*)text; *p; ++p)++symbol_freq[*p];
    count = nodes = 0;
    for (int ch = 0; ch < 256; ++ch)if (symbol_freq[ch]) {
        t->symbol[nodes] = ch;
        t->left[nodes] = t->right[nodes] = -1;
        freq[nodes] = symbol_freq[ch];
        active[count++] = nodes++;
    }
    if (!count) return 0;
    while (count > 1) {
        int a = 0, b = 1;
        if (freq[active[a]] > freq[active[b]]) {
            int tmp = a;
            a = b;
            b = tmp;
        }
        for (int i = 2; i < count; ++i) {
            if (c)c->comparisons++;
            if (freq[active[i]] < freq[active[a]]) {
                b = a;
                a = i;
            }
            else if (freq[active[i]] < freq[active[b]]) b = i;
        }
        int node = nodes++;
        t->symbol[node] = -1;
        t->left[node] = active[a];
        t->right[node] = active[b];
        freq[node] = freq[active[a]]+freq[active[b]];
        active[a] = node;
        active[b] = active[--count];
    }
    t->root = active[0];
    make_codes(t, t->root, path, 0, codes);
    for (const unsigned char *p = (const unsigned char*)text; *p; ++p) {
        size_t n = strlen(codes[*p]);
        if (bits+n >= cap) {
            errno = ENOSPC;
            out[0] = '\0';
            return -1;
        }
        memcpy(out+bits, codes[*p], n);
        bits += n;
    }
    out[bits] = '\0';
    return (int)bits;
}

int libraa_huffman_decode(const HuffmanTree*t, const char *bits, char *out, size_t cap) {
    if (!t || !bits || !out || !cap) {
        errno = EINVAL;
        return -1;
    }
    size_t count = 0;
    int at = t->root;
    out[0] = '\0';
    if (at < 0) return bits[0]?-1:0;
    for (size_t i = 0; bits[i]; ++i) {
        if (bits[i] != '0' && bits[i] != '1') return -1;
        if (t->left[t->root] >= 0) at = bits[i] == '0'?t->left[at]:t->right[at];
        else if (bits[i] != '0') return -1;
        if (at < 0 || at > 510) return -1;
        if (t->left[at] < 0) {
            if (count+1 >= cap) return -1;
            out[count++] = (char)t->symbol[at];
            at = t->root;
        }
    }
    if (at != t->root) return -1;
    out[count] = '\0';
    return (int)count;
}
/* Shared minimum-selection step for the two required greedy graph algorithms. */
static int minimum_unvisited(int n, const double *value, const bool *used, Counters *stats) {
    int best = -1;
    for (int i = 0; i < n; ++i) {
        if (stats)++stats->comparisons;
        if (!used[i] && isfinite(value[i]) && (best < 0 || value[i] < value[best])) best = i;
    }
    return best;
}

void libraa_dijkstra(int n, int src, const double *adj, double *out_dist, int *out_parent, Counters *c) {
    if (n < 1 || n > 128 || src < 0 || src >= n || !adj || !out_dist) {
        errno = EINVAL;
        return;
    }
    for (int i = 0; i < n*n; ++i)if (adj[i] < 0) {
        errno = EINVAL;
        return;
    }
    bool visited[128] = {
        false
    };
    for (int i = 0; i < n; ++i) {
        out_dist[i] = INFINITY;
        if (out_parent) out_parent[i] = -1;
    }
    out_dist[src] = 0.0;
    for (int count = 0; count < n - 1; ++count) {
        int u = minimum_unvisited(n, out_dist, visited, c);
        if (u == -1) break;
        visited[u] = true;
        for (int v = 0; v < n; ++v) {
            double weight = adj[u * n + v];
            if (c) c->comparisons++;
            if (!visited[v] && isfinite(weight) && out_dist[u] + weight < out_dist[v]) {
                out_dist[v] = out_dist[u] + weight;
                if (out_parent) out_parent[v] = u;
            }
        }
    }
}

double libraa_prim_mst(int n, const double *adj, int *out_parent, Counters *c) {
    if (n < 1 || n > 128 || !adj) {
        errno = EINVAL;
        return NAN;
    }
    for (int i = 0; i < n; ++i)for (int j = 0; j < n; ++j)if (adj[i*n+j] != adj[j*n+i]) {
        errno = EINVAL;
        return NAN;
    }
    double key[128];
    bool in_mst[128] = {
        false
    };
    for (int i = 0; i < n; ++i) {
        key[i] = INFINITY;
        if (out_parent) out_parent[i] = -1;
    }
    key[0] = 0.0;
    double total_weight = 0.0;
    for (int count = 0; count < n; ++count) {
        int u = minimum_unvisited(n, key, in_mst, c);
        if (u == -1 || !isfinite(key[u])) {
            errno = EINVAL;
            return NAN;
        }
        in_mst[u] = true;
        total_weight += key[u];
        for (int v = 0; v < n; ++v) {
            double weight = adj[u * n + v];
            if (c) c->comparisons++;
            if (isfinite(weight) && !in_mst[v] && weight < key[v]) {
                key[v] = weight;
                if (out_parent) out_parent[v] = u;
            }
        }
    }
    return total_weight;
}
// Disjoint Set for Kruskal
static int find_set(int parent[], int i) {
    if (parent[i] == i) return i;
    return parent[i] = find_set(parent, parent[i]);
}
typedef struct {
    int from, to;
    double weight;
} Edge;
static int compare_edges(const void *left, const void *right) {
    const Edge *a = left, *b = right;
    return (a->weight > b->weight)-(a->weight < b->weight);
}

double libraa_kruskal_mst(int n, int m, const int *u, const int *v, const double *weight, Counters *c) {
    if (n < 1 || n > 128 || m < 0 || m > 512 || !u || !v || !weight) {
        errno = EINVAL;
        return NAN;
    }
    Edge edges[512];
    int parent[128];
    for (int i = 0; i < m; ++i) {
        if (u[i] < 0 || u[i] >= n || v[i] < 0 || v[i] >= n || !isfinite(weight[i])) {
            errno = EINVAL;
            return NAN;
        }
        edges[i] = (Edge) {
            u[i], v[i], weight[i]
        };
    }
    if (!merge_sort(edges, m, sizeof(Edge), compare_edges, c)) return NAN;
    for (int i = 0; i < n; ++i)parent[i] = i;
    int taken = 0;
    double total = 0;
    for (int i = 0; i < m && taken < n-1; ++i) {
        int a = find_set(parent, edges[i].from), b = find_set(parent, edges[i].to);
        if (c)++c->comparisons;
        if (a != b) {
            parent[a] = b;
            total += edges[i].weight;
            ++taken;
        }
    }
    if (taken != n-1) {
        errno = EINVAL;
        return NAN;
    }
    return total;
}
// ============================================================================
// 3. DYNAMIC PROGRAMMING
// ============================================================================
int libraa_knapsack_01_dp(int n, int W, const int *weights, const int *values, int *selected, Counters *c) {
    if (n < 0 || n > 100 || W < 0 || W > 10000 || (n && (!weights || !values))) {
        errno = EINVAL;
        return -1;
    }
    for (int i = 0; i < n; ++i)if (weights[i] < 1 || values[i] < 0 || values[i] > 1000000) {
        errno = EINVAL;
        return -1;
    }
    size_t width = (size_t)W+1;
    int *dp = calloc(((size_t)n+1)*width, sizeof(int));
    if (!dp) {
        errno = ENOMEM;
        return -1;
    }
    counters_memory(c, ((size_t)n+1)*width*sizeof(int));
    for (int i = 1; i <= n; ++i)for (int w = 0; w <= W; ++w) {
        int best = dp[(i-1)*width+w];
        if (c)c->comparisons++;
        if (weights[i-1] <= w) {
            int take = values[i-1]+dp[(i-1)*width+w-weights[i-1]];
            if (take > best) best = take;
        }
        dp[i*width+w] = best;
    }
    int result = dp[n*width+W], w = W;
    if (selected) {
        memset(selected, 0, (size_t)n*sizeof(int));
        for (int i = n; i > 0; --i)if (dp[i*width+w] != dp[(i-1)*width+w]) {
            selected[i-1] = 1;
            w -= weights[i-1];
        }
    }
    free(dp);
    return result;
}

int libraa_matrix_chain_order(const int *p, int n, Counters *c) {
    if (!p || n < 1 || n > 30) {
        errno = EINVAL;
        return -1;
    }
    for (int i = 0; i <= n; ++i)if (p[i] < 1 || p[i] > 100) {
        errno = EINVAL;
        return -1;
    }
    // p has size n + 1 matrices
    int m[32][32] = {
        {0}
    };
    for (int L = 2; L <= n; ++L) {
        for (int i = 1; i <= n - L + 1; ++i) {
            int j = i + L - 1;
            m[i][j] = 1000000000;
            for (int k = i; k <= j - 1; ++k) {
                int q = m[i][k] + m[k + 1][j] + p[i - 1] * p[k] * p[j];
                if (c) {
                    c->comparisons++;
                    c->multiplications += 2;
                }
                if (q < m[i][j]) {
                    m[i][j] = q;
                }
            }
        }
    }
    return m[1][n];
}

double libraa_optimal_bst_roots(const double *p, const double *q, int n, int *roots, Counters *c) {
    if (!p || !q || n < 0 || n > 30) {
        errno = EINVAL;
        return NAN;
    }
    if (roots)for (int i = 0; i < (n+1)*(n+1); ++i)roots[i] = -1;
    for (int i = 0; i < n; ++i)if (p[i] < 0 || !isfinite(p[i])) {
        errno = EINVAL;
        return NAN;
    }
    for (int i = 0; i <= n; ++i)if (q[i] < 0 || !isfinite(q[i])) {
        errno = EINVAL;
        return NAN;
    }
    double e[32][32] = {
        {0}
    };
    double w[32][32] = {
        {0}
    };
    for (int i = 1; i <= n + 1; ++i) {
        e[i][i - 1] = q[i - 1];
        w[i][i - 1] = q[i - 1];
    }
    for (int l = 1; l <= n; ++l) {
        for (int i = 1; i <= n - l + 1; ++i) {
            int j = i + l - 1;
            e[i][j] = INFINITY;
            w[i][j] = w[i][j - 1] + p[j - 1] + q[j];
            for (int r = i; r <= j; ++r) {
                double t = e[i][r - 1] + e[r + 1][j] + w[i][j];
                if (c) c->comparisons++;
                /* Decimal probabilities have tiny representation differences across                    SSE/x87 compilers. Treat relative differences <=1e-12 as                    a tie;
                keep the first (lowest-index) root. */
                double tolerance = 1e-12*fmax(fabs(t), fabs(e[i][j]));
                if (!isfinite(e[i][j]) || t < e[i][j]-tolerance) {
                    e[i][j] = t;
                    if (roots)roots[i*(n+1)+j] = r;
                }
            }
        }
    }
    return e[1][n];
}

bool libraa_bellman_ford(int n, int m, const int *u, const int *v, const double *w, int src, double *out_dist, Counters *c) {
    if (n < 1 || n > 128 || m < 0 || src < 0 || src >= n || !out_dist || !u || !v || !w) {
        errno = EINVAL;
        return false;
    }
    for (int i = 0; i < m; ++i)if (u[i] < 0 || u[i] >= n || v[i] < 0 || v[i] >= n) {
        errno = EINVAL;
        return false;
    }
    for (int i = 0; i < n; ++i) out_dist[i] = INFINITY;
    out_dist[src] = 0.0;
    for (int i = 1; i <= n - 1; ++i) {
        for (int j = 0; j < m; ++j) {
            if (isfinite(out_dist[u[j]])) {
                if (c) c->comparisons++;
                if (out_dist[u[j]] + w[j] < out_dist[v[j]]) {
                    out_dist[v[j]] = out_dist[u[j]] + w[j];
                }
            }
        }
    }
    // Check negative-weight cycles
    for (int j = 0; j < m; ++j) {
        if (isfinite(out_dist[u[j]])) {
            if (out_dist[u[j]] + w[j] < out_dist[v[j]]) {
                return false;
                // Negative cycle detected
            }
        }
    }
    return true;
}

void libraa_floyd_warshall(int n, const double *adj, double *out_dist, Counters *c) {
    if (n<1||n>128||!adj||!out_dist) {
        errno=EINVAL;
        return;
    }
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            out_dist[i * n + j] = adj[i * n + j];
        }
    }
    for (int k = 0; k < n; ++k) {
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (isfinite(out_dist[i*n+k]) && isfinite(out_dist[k*n+j])) {
                    double via_k = out_dist[i * n + k] + out_dist[k * n + j];
                    if (c) c->comparisons++;
                    if (via_k < out_dist[i * n + j]) {
                        out_dist[i * n + j] = via_k;
                    }
                }
            }
        }
    }
    for (int i=0; i<n; ++i)if (out_dist[i*n+i]<0) {
        errno=EDOM;
        return;
    }
}

double libraa_tsp_dp(int n, const double *g, int *path, Counters *c) {
    if (n<1||n>12||!g) {
        errno=EINVAL;
        return NAN;
    }
    if (n==1) {
        if (path) {
            path[0]=path[1]=0;
        }
        return 0;
    }
    int states=1<<n, all=states-1;
    size_t size=(size_t)states*n;
    double *dp=malloc(size*sizeof(double));
    int *next=malloc(size*sizeof(int));
    if (!dp||!next) {
        free(dp);
        free(next);
        errno=ENOMEM;
        return NAN;
    }
    for (size_t i=0; i<size; ++i) {
        dp[i]=INFINITY;
        next[i]=-1;
    }
    for (int u=0; u<n; ++u)dp[all*n+u]=g[u*n];
    for (int mask=all-1; mask>=1; --mask)for (int u=0; u<n; ++u)if (mask&(1<<u))for (int v=0; v<n; ++v)if (!(mask&(1<<v))) {
        double cost=g[u*n+v]+dp[(mask|(1<<v))*n+v];
        if (c)c->comparisons++;
        if (cost<dp[mask*n+u]) {
            dp[mask*n+u]=cost;
            next[mask*n+u]=v;
        }
    }
    double result=dp[n];
    if (path) {
        for (int i=0; i<=n; ++i)path[i]=-1;
        if (isfinite(result)) {
            int mask=1, u=0;
            path[0]=0;
            for (int i=1; i<n; ++i) {
                u=next[mask*n+u];
                path[i]=u;
                mask|=1<<u;
            }
            path[n]=0;
        }
    }
    counters_memory(c, size*(sizeof(double)+sizeof(int)));
    free(dp);
    free(next);
    return result;
}
// ============================================================================
// 4. BACKTRACKING & BRANCH AND BOUND
// ============================================================================
static bool is_safe_nqueens(const int *board, int row, int col) {
    for (int i = 0; i < row; ++i) {
        if (board[i] == col || abs(board[i] - col) == abs(i - row)) {
            return false;
        }
    }
    return true;
}

static bool solve_nqueens_rec(int *board, int row, int n, Counters *c) {
    if (row == n) return true;
    for (int col = 0; col < n; ++col) {
        if (c) c->comparisons++;
        if (is_safe_nqueens(board, row, col)) {
            board[row] = col;
            if (solve_nqueens_rec(board, row + 1, n, c)) {
                return true;
            }
        }
    }
    return false;
}

int libraa_nqueens(int n, int *out_first_solution, Counters *c) {
    if (n < 1 || n > 12 || !out_first_solution) {
        errno = EINVAL;
        return -1;
    }
    int board[32];
    for (int i = 0; i < n; ++i) board[i] = -1;
    bool solved = solve_nqueens_rec(board, 0, n, c);
    if (solved && out_first_solution) {
        for (int i = 0; i < n; ++i) {
            out_first_solution[i] = board[i];
        }
        return 1;
    }
    return 0;
}

static bool subset_sum_rec(const int *set, int n, int idx, int curr_sum, int target_sum, int *sol, Counters *c) {
    if (curr_sum == target_sum) return true;
    if (idx >= n || curr_sum > target_sum) return false;
    if (c) c->comparisons++;
    sol[idx] = 1;
    if (subset_sum_rec(set, n, idx + 1, curr_sum + set[idx], target_sum, sol, c)) {
        return true;
    }
    sol[idx] = 0;
    return subset_sum_rec(set, n, idx + 1, curr_sum, target_sum, sol, c);
}

int libraa_sum_of_subsets(const int *set, int n, int target_sum, int *out_solution, Counters *c) {
    if (n < 0 || n > 24 || target_sum < 0 || !out_solution || !set) {
        errno = EINVAL;
        return -1;
    }
    for (int i = 0; i < n; ++i)if (set[i] < 0 || set[i] > 1000000) {
        errno = EINVAL;
        return -1;
    }
    for (int i = 0; i < n; ++i) out_solution[i] = 0;
    return subset_sum_rec(set, n, 0, 0, target_sum, out_solution, c) ? 1 : 0;
}
typedef struct {
    int id, weight, value;
    double ratio;
} BBItem;
typedef struct {
    int level, weight, value;
    double bound;
    int take[20];
} BBNode;
#define BB_CAPACITY 8192
static int compare_ratios(const void *left, const void *right) {
    const BBItem *a = left, *b = right;
    return (a->ratio < b->ratio)-(a->ratio > b->ratio);
}
/* Fractional relaxation is only a bound, not a second whole-book optimizer. */
static double upper_bound(const BBNode *node, int n, int W, const BBItem *items, Counters *s) {
    double value = node->value;
    int weight = node->weight;
    for (int j = node->level; j < n; ++j) {
        if (s)++s->comparisons;
        if (weight+items[j].weight <= W) {
            weight += items[j].weight;
            value += items[j].value;
        }
        else {
            value += (W-weight)*items[j].ratio;
            break;
        }
    }
    return value;
}
/* The only difference among FIFO/LIFO/best-bound is frontier access. */
static int push_node(BBNode *pending, int *count, int head, BBOrder order, BBNode node, Counters *s) {
    if (*count == BB_CAPACITY) {
        errno = E2BIG;
        return 0;
    }
    if (order == BB_FIFO)pending[(head+(*count)++)%BB_CAPACITY] = node;
    else if (order == BB_LIFO)pending[(*count)++] = node;
    else {
        int i = (*count)++;
        while (i > 0) {
            int parent = (i-1)/2;
            if (s)++s->comparisons;
            if (pending[parent].bound >= node.bound) break;
            pending[i] = pending[parent];
            i = parent;
        }
        pending[i] = node;
    }
    return 1;
}

static BBNode pop_node(BBNode *pending, int *count, int *head, BBOrder order, Counters *s) {
    if (order == BB_FIFO) {
        BBNode node = pending[*head];
        *head = (*head+1)%BB_CAPACITY;
        --*count;
        return node;
    }
    if (order == BB_LIFO) return pending[--*count];
    BBNode first = pending[0], last = pending[--*count];
    int i = 0;
    while (2*i+1 < *count) {
        int child = 2*i+1;
        if (child+1 < *count && pending[child+1].bound > pending[child].bound)++child;
        if (s)++s->comparisons;
        if (last.bound >= pending[child].bound) break;
        pending[i] = pending[child];
        i = child;
    }
    if (*count)pending[i] = last;
    return first;
}

int libraa_knapsack_01_bb(int n, int W, const int *weights, const int *values, int *selected, BBOrder order, Counters *s) {
    if (n < 0 || n > 20 || W < 0 || W > 10000 || (n && (!weights || !values)) || order < BB_FIFO || order > BB_BEST_BOUND) {
        errno = EINVAL;
        return -1;
    }
    BBItem items[20];
    for (int i = 0; i < n; ++i) {
        if (weights[i] < 1 || weights[i] > 10000 || values[i] < 0 || values[i] > 1000000) {
            errno = EINVAL;
            return -1;
        }
        items[i] = (BBItem) {
            i, weights[i], values[i], (double)values[i]/weights[i]
        };
    }
    if (selected)memset(selected, 0, (size_t)n*sizeof(int));
    if (!merge_sort(items, n, sizeof(BBItem), compare_ratios, s)) return -1;
    static BBNode pending[BB_CAPACITY];
    counters_memory(s, sizeof(pending)+sizeof(items));
    int count = 0, head = 0, best = 0;
    BBNode root = {0};
    root.bound = upper_bound(&root, n, W, items, s);
    if (!push_node(pending, &count, head, order, root, s)) return -1;
    while (count) {
        BBNode node = pop_node(pending, &count, &head, order, s);
        if (s)++s->comparisons;
        if (node.bound <= best || node.level >= n) continue;
        BBNode skip = node;
        skip.level++;
        skip.bound = upper_bound(&skip, n, W, items, s);
        if (skip.bound > best && !push_node(pending, &count, head, order, skip, s)) return -1;
        BBItem item = items[node.level];
        if (node.weight+item.weight <= W) {
            node.level++;
            node.weight += item.weight;
            node.value += item.value;
            node.take[item.id] = 1;
            if (node.value > best) {
                best = node.value;
                if (selected)memcpy(selected, node.take, (size_t)n*sizeof(int));
            }
            node.bound = upper_bound(&node, n, W, items, s);
            if (node.bound > best && !push_node(pending, &count, head, order, node, s)) return -1;
        }
    }
    return best;
}

static bool bfs_flow(int n, const int *r_graph, int s, int t, int *parent, Counters *c) {
    bool visited[64] = {
        false
    };
    int queue[64];
    int head = 0, tail = 0;
    queue[tail++] = s;
    visited[s] = true;
    parent[s] = -1;
    while (head < tail) {
        int u = queue[head++];
        for (int v = 0; v < n; ++v) {
            if (c) c->comparisons++;
            if (!visited[v] && r_graph[u * n + v] > 0) {
                if (v == t) {
                    parent[v] = u;
                    return true;
                }
                queue[tail++] = v;
                visited[v] = true;
                parent[v] = u;
            }
        }
    }
    return false;
}

int libraa_ford_fulkerson(int n, const int *cap_matrix, int src, int sink, Counters *c) {
    counters_memory(c, 64*64*sizeof(int)+4*64*sizeof(int));
    if (n < 2 || n > 64 || !cap_matrix || src < 0 || sink < 0 || src >= n || sink >= n || src == sink) {
        errno = EINVAL;
        return -1;
    }
    for (int i = 0; i < n*n; ++i)if (cap_matrix[i] < 0 || cap_matrix[i] > 1000000) {
        errno = EINVAL;
        return -1;
    }
    int r_graph[64 * 64];
    for (int i = 0; i < n * n; ++i) {
        r_graph[i] = cap_matrix[i];
    }
    int parent[64];
    int max_flow = 0;
    while (bfs_flow(n, r_graph, src, sink, parent, c)) {
        int path_flow = 1000000000;
        for (int v = sink; v != src; v = parent[v]) {
            int u = parent[v];
            if (r_graph[u * n + v] < path_flow) {
                path_flow = r_graph[u * n + v];
            }
        }
        for (int v = sink; v != src; v = parent[v]) {
            int u = parent[v];
            r_graph[u * n + v] -= path_flow;
            r_graph[v * n + u] += path_flow;
        }
        max_flow += path_flow;
    }
    return max_flow;
}

static bool bpm_dfs(int u, int right_count, const int *adj, bool* seen, int *matchR, Counters *c) {
    for (int v = 0; v < right_count; ++v) {
        if (c) c->comparisons++;
        if (adj[u * right_count + v] && !seen[v]) {
            seen[v] = true;
            if (matchR[v] < 0 || bpm_dfs(matchR[v], right_count, adj, seen, matchR, c)) {
                matchR[v] = u;
                return true;
            }
        }
    }
    return false;
}

int libraa_max_bipartite_matching(int left_count, int right_count, const int *adj, int *out_match, Counters *c) {
    counters_memory(c, 64*(sizeof(int)+sizeof(bool)));
    if (left_count < 0 || right_count < 0 || left_count > 64 || right_count > 64 || !adj) {
        errno = EINVAL;
        return -1;
    }
    int matchR[64];
    for (int i = 0; i < right_count; ++i) matchR[i] = -1;
    int max_matches = 0;
    for (int u = 0; u < left_count; ++u) {
        bool seen[64] = {
            false
        };
        if (bpm_dfs(u, right_count, adj, seen, matchR, c)) {
            max_matches++;
        }
    }
    if (out_match) {
        for (int v = 0; v < right_count; ++v) {
            out_match[v] = matchR[v];
        }
    }
    return max_matches;
}
// ============================================================================
// 6. NUMBER THEORY (CHINESE REMAINDER THEOREM)
// ============================================================================
static long long extended_gcd(long long a, long long b, long long *x, long long *y) {
    if (a == 0) {
        *x = 0;
        *y = 1;
        return b;
    }
    long long x1, y1;
    long long gcd = extended_gcd(b % a, a, &x1, &y1);
    *x = y1 - (b / a) * x1;
    *y = x1;
    return gcd;
}

static long long mod_inverse(long long a, long long m) {
    long long x, y;
    long long g = extended_gcd(a, m, &x, &y);
    if (g != 1) return -1;
    return (x % m + m) % m;
}

long long libraa_chinese_remainder_theorem(const long long *num, const long long *rem, int k, Counters *c) {
    if (!num || !rem || k < 1) {
        errno = EINVAL;
        return -1;
    }
    long long prod = 1, result = 0;
    for (int i = 0; i < k; ++i) {
        if (num[i] < 2 || prod > 1000000000LL/num[i]) {
            errno = EINVAL;
            return -1;
        }
        prod *= num[i];
    }
    for (int i = 0; i < k; ++i) {
        long long pp = prod/num[i], inv = mod_inverse(pp, num[i]);
        if (inv < 0) {
            errno = EINVAL;
            return -1;
        }
        long long r = (rem[i]%num[i]+num[i])%num[i];
        result = (result+(r*pp%prod)*inv%prod)%prod;
        if (c)c->multiplications += 2;
    }
    return result;
}
