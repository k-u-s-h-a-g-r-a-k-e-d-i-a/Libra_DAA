#include "libra.h"
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif
static int checks, failures;
#define CHECK(condition,label) do { ++checks; if(!(condition)){++failures;fprintf(out,"FAIL: %s (line %d)\n",label,__LINE__);} } while(0)
static unsigned state;
static unsigned random_number(void) {
    state=state*1664525u+1013904223u;
    return state;
}

static int brute_knapsack(int n, int W, const int *wt, const int *val) {
    int best=0;
    for (int mask=0; mask<(1<<n); ++mask) {
        int w=0, v=0;
        for (int i=0; i<n; ++i)if (mask&(1<<i)) {
            w+=wt[i];
            v+=val[i];
        }
        if (w<=W&&v>best) best=v;
    }
    return best;
}

static int brute_matching(int left, int right, const int *edges, int reader, int used) {
    if (reader==left) return 0;
    int best=brute_matching(left, right, edges, reader+1, used);
    for (int j=0; j<right; ++j)if (edges[reader*right+j]&&!(used&(1<<j))) {
        int v=1+brute_matching(left, right, edges, reader+1, used|(1<<j));
        if (v>best) best=v;
    }
    return best;
}

static int feasible(int n, int W, const int *wt, const int *val, const int *chosen, int result) {
    int w=0, v=0;
    for (int i=0; i<n; ++i) {
        if (chosen[i]!=0&&chosen[i]!=1) return 0;
        if (chosen[i]) {
            w+=wt[i];
            v+=val[i];
        }
    }
    return v==result&&w<=W;
}
/* Verify the returned BST itself, independent of the DP cost table. */
static double bst_witness_cost(const double *p, const double *q, const int *roots, int lo, int hi, int depth, int stride) {
    if (lo > hi) return q[hi]*depth;
    int r = roots[lo*stride+hi];
    if (r < lo || r > hi) return NAN;
    return p[r-1]*depth         +bst_witness_cost(p, q, roots, lo, r-1, depth+1, stride)         +bst_witness_cost(p, q, roots, r+1, hi, depth+1, stride);
}
typedef struct {
    int priority, id;
    char payload[17];
} SortExample;
static int compare_priority(const void *left, const void *right) {
    const SortExample *a = left, *b = right;
    return (a->priority > b->priority)-(a->priority < b->priority);
}

int run_tests(FILE *out) {
    checks = failures = 0;
    SortExample records[] = {
        {
            2, 0,"first"
        }, {
            1, 1,"second"
        }, {
            2, 2,"third"
        }, {
            1, 3,"fourth"
        }
    };
    CHECK(merge_sort(records, 4, sizeof(*records), compare_priority, NULL),"shared merge supports records");
    CHECK(records[0].id == 1 && records[1].id == 3 && records[2].id == 0 && records[3].id == 2,"stable record sorting preserves ties and payload");
    CHECK(strcmp(records[0].payload,"second") == 0 && strcmp(records[3].payload,"third") == 0,"record bytes preserved");
    SortExample key = {
        2, 0, {0}
    };
    int found = binary_iter(records, 4, sizeof(*records), &key, compare_priority, NULL);
    CHECK(found >= 0 && records[found].priority == 2,"shared binary search supports records");
    CHECK(quick_sort(records, 4, sizeof(*records), compare_priority, NULL),"shared quick supports records");
    CHECK(records[0].priority == 1 && records[3].priority == 2,"record quick ordering");
    CHECK(!merge_sort(records, 4, 0, compare_priority, NULL),"zero-width sort rejected");
    Activity same_id[] = {{7, 0, 3}, {7, 1, 4}};
    int rooms[] = {-1, -1};
    CHECK(libraa_interval_partitioning(same_id, 2, rooms, NULL) == 2 &&
          rooms[0] == 1 && rooms[1] == 2, "overlapping events with equal IDs get separate rooms");

    int from[] = {0}, to[] = {1};
    double bad_weight[] = {NAN};
    errno = 0;
    CHECK(isnan(libraa_kruskal_mst(2, 1, from, to, bad_weight, NULL)) && errno == EINVAL,
          "reject a nonfinite edge weight");

    double matrix[] = {1, 2, 3, 4}, product[4];
    Counters matrix_stats = {0};
    libraa_matrix_multiply_strassen(2, matrix, matrix, product, &matrix_stats);
    CHECK(matrix_stats.aux_bytes == (12 + 17) * sizeof(double), "padded and recursive matrix buffers overlap in time");
    state = 17;
    for (int trial = 0; trial < 150; ++trial) {
        char text[65], pattern[12];
        int n = trial%64, m = trial%11;
        for (int i = 0; i < n; ++i)text[i] = (char)('a'+random_number()%3);
        text[n] = '\0';
        for (int i = 0; i < m; ++i)pattern[i] = (char)('a'+random_number()%3);
        pattern[m] = '\0';
        size_t oracle[65], actual[65], expected = match_text(text, pattern, NAIVE, oracle, 65, NULL);
        for (int e = 1; e < 4; ++e) {
            size_t count = match_text(text, pattern, (Matcher)e, actual, 65, NULL);
            CHECK(count == expected && memcmp(oracle, actual, count*sizeof(size_t)) == 0,"matcher oracle positions");
        }
    }
    for (int e = 0; e < 4; ++e) {
        size_t p[3];
        CHECK(match_text("aaaa","aa", (Matcher)e, p, 3, NULL) == 3 && p[0] == 0 && p[1] == 1 && p[2] == 2,"overlapping matches");
    }
    for (int n = 0; n < 80; ++n) {
        int a[80], b[80], counts[10] = {0}, after[10] = {0};
        for (int i = 0; i < n; ++i) {
            a[i] = b[i] = (int)(random_number()%10);
            ++counts[a[i]];
        }
        CHECK(merge_sort(a, n, sizeof(int), compare_ints, NULL),"merge succeeds");
        quick_sort(b, n, sizeof(int), compare_ints, NULL);
        CHECK(memcmp(a, b, (size_t)n*sizeof(int)) == 0,"sort agreement");
        for (int i = 0; i < n; ++i) {
            ++after[a[i]];
            if (i)CHECK(a[i-1] <= a[i],"sorted order");
        }
        CHECK(memcmp(counts, after, sizeof(counts)) == 0,"multiset preserved");
        for (int key = -1; key <= 10; ++key) {
            int x = binary_iter(a, n, sizeof(int), &key, compare_ints, NULL), y = binary_rec(a, n, sizeof(int), &key, compare_ints, NULL), present = 0;
            for (int i = 0; i < n; ++i)if (a[i] == key) present = 1;
            CHECK((x >= 0) == present && (y >= 0) == present,"binary hit/miss oracle");
        }
    }
    char subseq[256];
    CHECK(lcs("data","dart", subseq, sizeof(subseq), NULL) == 3 && strcmp(subseq,"dat") == 0,"LCS reconstruction");
    CHECK(lcs("","", subseq, sizeof(subseq), NULL) == 0,"empty LCS");
    CHECK(lcs("abc","abc", subseq, 2, NULL) == -1,"small LCS buffer rejected");
    for (int trial = 1; trial < 60; ++trial) {
        int n = 1+trial%15, a[16], best = -1000000, s, e;
        for (int i = 0; i < n; ++i)a[i] = (int)(random_number()%21)-10;
        for (int i = 0; i < n; ++i) {
            int sum = 0;
            for (int j = i; j < n; ++j) {
                sum += a[j];
                if (sum > best) best = sum;
            }
        }
        CHECK(libraa_max_subarray_dc(a, 0, n-1, &s, &e, NULL) == best,"subarray exhaustive oracle");
    }
    CHECK(libraa_fast_pow_mod(2, 10, 1000, NULL) == 24 && libraa_fast_pow_mod(-2, 3, 5, NULL) == 2 && libraa_fast_pow_mod(2, 0, 1, NULL) == 0,"modular power");
    CHECK(libraa_fast_pow_mod(2, 3, 0, NULL) == -1,"invalid modulus");
    int celeb[] = {0, 1, 1, 0, 0, 1, 0, 0, 0}, none[] = {0, 0, 0, 0};
    CHECK(libraa_celebrity_find(celeb, 3, NULL) == 2 && libraa_celebrity_find(none, 2, NULL) == -1,"celebrity verification");
    for (int n = 1; n <= 11; ++n) {
        double A[121], B[121], C[121], S[121];
        for (int i = 0; i < n*n; ++i) {
            A[i] = random_number()%6;
            B[i] = random_number()%6;
        }
        errno = 0;
        libraa_matrix_multiply_classical(n, A, B, C, NULL);
        libraa_matrix_multiply_strassen(n, A, B, S, NULL);
        CHECK(errno == 0 && memcmp(C, S, (size_t)n*n*sizeof(double)) == 0,"padded recursive Strassen");
    }
    Activity slots[] = {
        {1, 1, 4}, {2, 2, 5}, {3, 4, 7}, {4, 3, 6}
    };
    int selected[4], room[4];
    CHECK(libraa_activity_selection(slots, 4, selected, NULL) == 2,"activity selection");
    CHECK(libraa_interval_partitioning(slots, 4, room, NULL) == 3,"partition optimum");
    for (int i = 0; i < 4; ++i)for (int j = 0; j < i; ++j)if (room[i] == room[j])CHECK(slots[i].start >= slots[j].finish || slots[j].start >= slots[i].finish,"room feasibility");
    for (int trial = 0; trial < 100; ++trial) {
        int n = 1+trial%9, W = (int)(random_number()%25), wt[9], val[9], take[9];
        for (int i = 0; i < n; ++i) {
            wt[i] = 1+(int)(random_number()%8);
            val[i] = (int)(random_number()%20);
        }
        int expected = brute_knapsack(n, W, wt, val);
        int result = libraa_knapsack_01_dp(n, W, wt, val, take, NULL);
        CHECK(result == expected && feasible(n, W, wt, val, take, result),"DP exact witness");
        result = libraa_knapsack_01_bb(n, W, wt, val, take, BB_FIFO, NULL);
        CHECK(result == expected && feasible(n, W, wt, val, take, result),"FIFO exact witness");
        result = libraa_knapsack_01_bb(n, W, wt, val, take, BB_LIFO, NULL);
        CHECK(result == expected && feasible(n, W, wt, val, take, result),"LIFO exact witness");
        result = libraa_knapsack_01_bb(n, W, wt, val, take, BB_BEST_BOUND, NULL);
        CHECK(result == expected && feasible(n, W, wt, val, take, result),"best-bound exact witness");
    }
    int invalid_wt[] = {0}, valid_val[] = {5}, take[1];
    CHECK(libraa_knapsack_01_dp(1, 5, invalid_wt, valid_val, take, NULL) == -1,"invalid knapsack item");
    const char *texts[] = {
        "","a","aaaa","abcd","hello library","\"quotes\" and commas,"
    };
    for (int i = 0; i < 6; ++i) {
        HuffmanTree h;
        char bits[8192], decoded[2049];
        int n = libraa_huffman_encode(texts[i], bits, sizeof(bits), &h, NULL);
        CHECK(n >= 0 && libraa_huffman_decode(&h, bits, decoded, sizeof(decoded)) >= 0 && strcmp(texts[i], decoded) == 0,"Huffman round trip");
        if (i == 2)CHECK(n == 4,"singleton Huffman payload");
        if (i == 3)CHECK(n == 8,"four-symbol Huffman payload");
    }
    double g[] = {
        0, 7, 9, INFINITY, INFINITY, 14, 7, 0, 10, 15, INFINITY, INFINITY, 9, 10, 0, 11, INFINITY, 2, INFINITY, 15, 11, 0, 6, INFINITY, INFINITY, INFINITY, INFINITY, 6, 0, 9, 14, INFINITY, 2, INFINITY, 9, 0
    };
    double all[36], dist[6], bf[6], ew[36];
    int u[36], v[36], parent[6], m = 0;
    for (int i = 0; i < 6; ++i)for (int j = 0; j < 6; ++j)if (i != j && isfinite(g[i*6+j])) {
        u[m] = i;
        v[m] = j;
        ew[m++] = g[i*6+j];
    }
    libraa_floyd_warshall(6, g, all, NULL);
    for (int src = 0; src < 6; ++src) {
        libraa_dijkstra(6, src, g, dist, parent, NULL);
        int valid = libraa_bellman_ford(6, m, u, v, ew, src, bf, NULL);
        for (int i = 0; i < 6; ++i)CHECK(valid && dist[i] == bf[i] && dist[i] == all[src*6+i],"shortest-path cross-oracles");
    }
    CHECK(all[5] == 11,"missing-edge regression");
    m = 0;
    for (int i = 0; i < 6; ++i)for (int j = i+1; j < 6; ++j)if (isfinite(g[i*6+j])) {
        u[m] = i;
        v[m] = j;
        ew[m++] = g[i*6+j];
    }
    CHECK(libraa_prim_mst(6, g, parent, NULL) == 33 && libraa_kruskal_mst(6, m, u, v, ew, NULL) == 33,"MST cross-oracles");
    double zero[] = {0, 0, 5, 0, 0, 2, 5, 2, 0};
    libraa_dijkstra(3, 0, zero, dist, NULL, NULL);
    CHECK(dist[2] == 2,"zero-cost edge");
    int nu[] = {0, 1}, nv[] = {1, 0};
    double nw[] = {-2, 1};
    CHECK(!libraa_bellman_ford(2, 2, nu, nv, nw, 0, dist, NULL),"reachable negative cycle");
    double disconnected[] = {
        0, INFINITY, INFINITY, 0
    };
    CHECK(isnan(libraa_prim_mst(2, disconnected, NULL, NULL)),"disconnected MST rejected");
    int dims[] = {10, 20, 30, 40, 30};
    CHECK(libraa_matrix_chain_order(dims, 4, NULL) == 30000,"matrix chain");
    double hit[] = {.15, .10, .05, .10, .20}, miss[] = {.05, .10, .05, .05, .05, .10};
    int roots[36];
    double bst_cost = libraa_optimal_bst_roots(hit, miss, 5, roots, NULL);
    CHECK(fabs(bst_cost-2.75) < 1e-9,"OBST optimal cost");
    CHECK(fabs(bst_witness_cost(hit, miss, roots, 1, 5, 1, 6)-bst_cost) < 1e-9,"OBST returned tree achieves reported cost");
    CHECK(roots[11] == 2,"OBST near-equal ties keep the first root deterministically");
    for (int n = 1; n <= 12; ++n) {
        double d[144];
        int path[13], seen[12] = {0};
        for (int i = 0; i < n; ++i)for (int j = 0; j < n; ++j)d[i*n+j] = i == j?0:1;
        double cost = libraa_tsp_dp(n, d, path, NULL), actual = 0;
        int valid = 1;
        for (int i = 0; i < n; ++i) {
            if (path[i] < 0 || path[i] >= n) {
                valid = 0;
                break;
            }
            ++seen[path[i]];
            actual += d[path[i]*n+path[i+1]];
        }
        for (int i = 0; i < n; ++i)if (seen[i] != 1) valid = 0;
        CHECK(valid && cost == actual && path[n] == 0,"TSP route/cost including n11/n12");
    }
    double tour[] = {0, 10, 15, 20, 10, 0, 35, 25, 15, 35, 0, 30, 20, 25, 30, 0};
    int route[5];
    double cost = libraa_tsp_dp(4, tour, route, NULL), actual = 0;
    for (int i = 0; i < 4; ++i)actual += tour[route[i]*4+route[i+1]];
    CHECK(cost == 80 && actual == 80,"TSP reconstructed optimal route");
    CHECK(isnan(libraa_tsp_dp(13, tour, route, NULL)),"TSP size bound");
    int board[8];
    CHECK(libraa_nqueens(8, board, NULL) == 1,"8 queens solved");
    for (int i = 0; i < 8; ++i)for (int j = 0; j < i; ++j)CHECK(board[i] != board[j] && abs(board[i]-board[j]) != i-j,"queen constraints");
    CHECK(libraa_nqueens(2, board, NULL) == 0,"no two-queen solution");
    int set[] = {10, 7, 5, 18, 12, 20, 15}, solution[7], sum = 0;
    libraa_sum_of_subsets(set, 7, 35, solution, NULL);
    for (int i = 0; i < 7; ++i)if (solution[i])sum += set[i];
    CHECK(sum == 35,"subset witness");
    int cap[] = {0, 10, 10, 0, 0, 0, 2, 8, 0, 0, 0, 9, 0, 0, 0, 0};
    CHECK(libraa_ford_fulkerson(4, cap, 0, 3, NULL) == 17,"max-flow oracle");
    for (int trial = 0; trial < 40; ++trial) {
        int edges[12], owner[3], cap2[81] = {0};
        for (int i = 0; i < 12; ++i)edges[i] = (int)(random_number()%2);
        int expected = brute_matching(4, 3, edges, 0, 0), count = libraa_max_bipartite_matching(4, 3, edges, owner, NULL), used[4] = {0};
        for (int j = 0; j < 3; ++j)if (owner[j] >= 0) {
            CHECK(edges[owner[j]*3+j] && !used[owner[j]],"matching feasible");
            used[owner[j]] = 1;
        }
        for (int i = 0; i < 4; ++i) {
            cap2[i+1] = 1;
            for (int j = 0; j < 3; ++j)cap2[(i+1)*9+5+j] = edges[i*3+j];
        }
        for (int j = 0; j < 3; ++j)cap2[(5+j)*9+8] = 1;
        CHECK(count == expected && libraa_ford_fulkerson(9, cap2, 0, 8, NULL) == expected,"matching/flow/exhaustive oracle");
    }
    int hard_wt[20], hard_val[20], hard_take[20];
    for (int i = 0; i < 20; ++i) {
        hard_wt[i] = 2;
        hard_val[i] = 3;
    }
    errno = 0;
    int hard = libraa_knapsack_01_bb(20, 19, hard_wt, hard_val, hard_take, BB_FIFO, NULL);
    CHECK(hard == -1 && errno == E2BIG,"FIFO exhaustion returns failure instead of a false optimum");
    double negative_cycle[] = {0, -2, 1, 0}, negative_result[4];
    errno = 0;
    libraa_floyd_warshall(2, negative_cycle, negative_result, NULL);
    CHECK(errno == EDOM,"Floyd negative cycle reported");
    long long mods[] = {3, 5, 7}, rems[] = {2, 3, 2}, badmods[] = {4, 6};
    CHECK(libraa_chinese_remainder_theorem(mods, rems, 3, NULL) == 23,"CRT");
    CHECK(libraa_chinese_remainder_theorem(badmods, rems, 2, NULL) == -1,"noncoprime CRT rejected");
    Library lib, copy;
    char error[256];
    library_init(&lib);
    library_init(&copy);
    CHECK(library_load(&lib,"data/books.csv", error, sizeof(error)),"sample catalog loads");
    if (lib.count) {
        CHECK(find_book(&lib, 1, 0) == find_book(&lib, 1, 1) && find_book(&lib, 1, 0) != NULL && !find_book(&lib, -1, 0),"book binary lookup");
        int ids[10];
        CHECK(keyword_lookup(&lib,"sorting", ids, 10) > 0,"keyword index");
        Hit a[10], b[10];
        CHECK(search_books(&lib, "sorting", (Matcher)99, a, 10, NULL) == -1, "reject an unknown matching algorithm");
        int na = search_books(&lib,"sorting", NAIVE, a, 10, NULL), nb = search_books(&lib,"sorting", KMP, b, 10, NULL);
        CHECK(na == nb && na > 0,"catalog search count");
        for (int i = 0; i < na; ++i)CHECK(a[i].id == b[i].id && a[i].score == b[i].score,"same ranked results");
        CHECK(search_books(&lib,"   ", KMP, a, 10, NULL) == -1,"empty query rejected");
        CHECK(recommend_books(&lib, 1, a, 10) > 0,"recommendation candidates");
        make_directory("results");
        char folder[128], path[256];
        snprintf(folder, sizeof(folder),"results/test_tmp_%lu", (unsigned long)clock());
        CHECK(export_reports(&lib, folder),"CSV export");
        snprintf(path, sizeof(path),"%s/catalog.csv", folder);
        CHECK(library_load(&copy, path, error, sizeof(error)) && copy.count == lib.count,"CSV round trip");
        snprintf(path, sizeof(path),"%s/bad.csv", folder);
        FILE *file = fopen(path,"w");
        if (file) {
            fprintf(file,"id,title,author,subject,keywords,description,copies,hours,value\n1,x,a,s,k,d,-1,2,3\n");
            fclose(file);
            int before = lib.count;
            CHECK(!library_load(&lib, path, error, sizeof(error)) && lib.count == before,"transactional invalid load");
        }
        else CHECK(0,"write invalid fixture");
        snprintf(path, sizeof(path),"%s/quotes.csv", folder);
        file = fopen(path,"w");
        if (file) {
            fprintf(file,"id,title,author,subject,keywords,description,copies,hours,value\n99,\"a, \"\"quoted\"\" book\",a,s,k,d,1,2,3\n");
            fclose(file);
            CHECK(library_load(&copy, path, error, sizeof(error)) && strcmp(copy.books[0].title,"a, \"quoted\" book") == 0,"CSV quoted fields");
        }
        else CHECK(0,"write quoted fixture");
        const char *files[] = {
            "catalog.csv","index.csv","recommendations.csv","relationships.csv","bad.csv","quotes.csv"
        };
        for (int i = 0; i < 6; ++i) {
            snprintf(path, sizeof(path),"%s/%s", folder, files[i]);
            remove(path);
        }
#ifdef _WIN32
        _rmdir(folder);
#else
        rmdir(folder);
#endif
    }
    library_free(&lib);
    library_free(&copy);
    fprintf(out,"%s: %d assertions, %d failures. Seeded independent oracles, witnesses, edge cases and regression tests.\n", failures?"FAIL":"PASS", checks, failures);
    return failures == 0;
}

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
static int check_choice(int n, int W, const int *weights, const int *values, const int *chosen, int result) {
    int weight = 0, value = 0;
    if (result < 0) return 0;
    for (int i = 0; i < n; ++i) {
        if (chosen[i] != 0 && chosen[i] != 1) return 0;
        if (chosen[i]) {
            weight += weights[i];
            value += values[i];
        }
    }
    return weight <= W && value == result;
}

static int choose(int method, int n, int W, const int *weights, const int *values, int *selected, Counters *stats) {
    if (method == 0) return libraa_knapsack_01_dp(n, W, weights, values, selected, stats);
    return libraa_knapsack_01_bb(n, W, weights, values, selected, (BBOrder)(method-1), stats);
}

static int flow_assignment(int left, int right, const int *edges, Counters *s) {
    int n = left+right+2, sink = n-1;
    int *cap = calloc((size_t)n*n, sizeof(int));
    if (!cap) return -1;
    for (int i = 0; i < left; ++i) {
        cap[i+1] = 1;
        for (int j = 0; j < right; ++j)cap[(i+1)*n+left+1+j] = edges[i*right+j];
    }
    for (int j = 0; j < right; ++j)cap[(left+1+j)*n+sink] = 1;
    int flow = libraa_ford_fulkerson(n, cap, 0, sink, s);
    free(cap);
    return flow;
}

static void print_tree(FILE *out, const int *root, int lo, int hi, int depth, int stride) {
    if (lo > hi) return;
    int r = root[lo*stride+hi];
    fprintf(out,"%*skeyword[%d]\n", depth*2,"", r);
    print_tree(out, root, lo, r-1, depth+1, stride);
    print_tree(out, root, r+1, hi, depth+1, stride);
}

static void record_algorithm(FILE *csv, const char *name, const Timer *timer, const Counters *stats) {
    if (csv) fprintf(csv, "%s,%lld,%lld,%lld,%lld\n", name, timer_stop_us(timer),
                     stats->comparisons, stats->multiplications, stats->aux_bytes);
}

static int run_examples(FILE *out, FILE *csv) {
    Timer measurement_timer;
    Counters measurement_stats;
    fprintf(out,"ALGORITHM LAB: required analytical examples, not extra app features\n");
    int activity[] = {-2, 1, -3, 4, -1, 2, 1, -5, 4}, first, last;
    counters_reset(&measurement_stats);
    timer_start(&measurement_timer);
    int peak = libraa_max_subarray_dc(activity, 0, 8, &first, &last, &measurement_stats);
    record_algorithm(csv, "libraa_max_subarray_dc", &measurement_timer, &measurement_stats);
    fprintf(out,"Peak borrowing window, divide and conquer=%d\n", peak);
    if (peak != 6) return 0;
    counters_reset(&measurement_stats);
    timer_start(&measurement_timer);
    long long power = libraa_fast_pow_mod(2, 10, 1000, &measurement_stats);
    record_algorithm(csv, "libraa_fast_pow_mod", &measurement_timer, &measurement_stats);
    fprintf(out,"Fast power: 2^10 mod 1000=%lld\n", power);
    if (power != 24) return 0;
    int citations[] = {0, 1, 1, 0, 0, 1, 0, 0, 0};
    counters_reset(&measurement_stats);
    timer_start(&measurement_timer);
    int celebrity = libraa_celebrity_find(citations, 3, &measurement_stats);
    record_algorithm(csv, "libraa_celebrity_find", &measurement_timer, &measurement_stats);
    fprintf(out,"Universally cited document (zero-based)=%d\n", celebrity);
    if (celebrity != 2) return 0;
    double A[] = {1, 2, 3, 4}, B[] = {5, 6, 7, 8}, C[4], S[4];
    Counters cc, cs;
    counters_reset(&cc);
    counters_reset(&cs);
    errno = 0;
    timer_start(&measurement_timer);
    libraa_matrix_multiply_classical(2, A, B, C, &cc);
    record_algorithm(csv, "libraa_matrix_multiply_classical", &measurement_timer, &cc);
    timer_start(&measurement_timer);
    libraa_matrix_multiply_strassen(2, A, B, S, &cs);
    record_algorithm(csv, "libraa_matrix_multiply_strassen", &measurement_timer, &cs);
    if (errno == ENOMEM) return 0;
    int same = memcmp(C, S, sizeof(C)) == 0;
    fprintf(out,"Similarity matrix products agree=%d; scalar multiplications classical=%lld, Strassen=%lld\n", same, cc.multiplications, cs.multiplications);
    Activity slots[] = {
        {1, 1, 4}, {2, 2, 5}, {3, 4, 7}, {4, 3, 6}
    };
    int selected[4], rooms[4];
    counters_reset(&measurement_stats);
    timer_start(&measurement_timer);
    int scheduled = libraa_activity_selection(slots, 4, selected, &measurement_stats);
    record_algorithm(csv, "libraa_activity_selection", &measurement_timer, &measurement_stats);
    counters_reset(&measurement_stats);
    timer_start(&measurement_timer);
    int room_count = libraa_interval_partitioning(slots, 4, rooms, &measurement_stats);
    record_algorithm(csv, "libraa_interval_partitioning", &measurement_timer, &measurement_stats);
    fprintf(out,"Reading-room events: %d non-overlapping selections; %d rooms to hold all\n", scheduled, room_count);
    if (scheduled != 2 || room_count != 3) return 0;
    HuffmanTree tree;
    char bits[8192], decoded[2049];
    const char *text = "algorithms and digital library";
    counters_reset(&measurement_stats);
    timer_start(&measurement_timer);
    int bit_count = libraa_huffman_encode(text, bits, sizeof(bits), &tree, &measurement_stats);
    record_algorithm(csv, "libraa_huffman_encode", &measurement_timer, &measurement_stats);
    if (bit_count < 0) return 0;
    int recovered = libraa_huffman_decode(&tree, bits, decoded, sizeof(decoded));
    fprintf(out,"Huffman: %zu input bits -> %d payload bits; decoded correctly=%d\n", strlen(text)*8, bit_count, recovered >= 0 && strcmp(text, decoded) == 0);
    fprintf(out,"Bits stored as characters for teaching; codebook/padding overhead excluded, not packed-file compression.\n");
    if (recovered < 0 || strcmp(text, decoded) != 0) return 0;
    double g[] = {
        0, 7, 9, INFINITY, INFINITY, 14, 7, 0, 10, 15, INFINITY, INFINITY, 9, 10, 0, 11, INFINITY, 2, INFINITY, 15, 11, 0, 6, INFINITY, INFINITY, INFINITY, INFINITY, 6, 0, 9, 14, INFINITY, 2, INFINITY, 9, 0
    };
    double distances[6], all[36], bf[6];
    int parent[6], u[36], v[36], edges = 0;
    double edge_weights[36];
    for (int i = 0; i < 6; ++i)for (int j = 0; j < 6; ++j)if (i != j && isfinite(g[i*6+j])) {
        u[edges] = i;
        v[edges] = j;
        edge_weights[edges++] = g[i*6+j];
    }
    counters_reset(&measurement_stats);
    timer_start(&measurement_timer);
    libraa_dijkstra(6, 0, g, distances, parent, &measurement_stats);
    record_algorithm(csv, "libraa_dijkstra", &measurement_timer, &measurement_stats);
    counters_reset(&measurement_stats);
    timer_start(&measurement_timer);
    libraa_bellman_ford(6, edges, u, v, edge_weights, 0, bf, &measurement_stats);
    record_algorithm(csv, "libraa_bellman_ford", &measurement_timer, &measurement_stats);
    counters_reset(&measurement_stats);
    timer_start(&measurement_timer);
    libraa_floyd_warshall(6, g, all, &measurement_stats);
    record_algorithm(csv, "libraa_floyd_warshall", &measurement_timer, &measurement_stats);
    fprintf(out,"Branch route 0->5: Dijkstra=%.0f, Bellman-Ford=%.0f, Floyd=%.0f\n", distances[5], bf[5], all[5]);
    for (int i = 0; i < 6; i++) {
        if (distances[i] != bf[i] || distances[i] != all[i]) return 0;
    }
    if (distances[5] != 11) return 0;
    edges = 0;
    for (int i = 0; i < 6; ++i)for (int j = i+1; j < 6; ++j)if (isfinite(g[i*6+j])) {
        u[edges] = i;
        v[edges] = j;
        edge_weights[edges++] = g[i*6+j];
    }
    counters_reset(&measurement_stats);
    timer_start(&measurement_timer);
    double prim = libraa_prim_mst(6, g, parent, &measurement_stats);
    record_algorithm(csv, "libraa_prim_mst", &measurement_timer, &measurement_stats);
    counters_reset(&measurement_stats);
    timer_start(&measurement_timer);
    double kruskal = libraa_kruskal_mst(6, edges, u, v, edge_weights, &measurement_stats);
    record_algorithm(csv, "libraa_kruskal_mst", &measurement_timer, &measurement_stats);
    fprintf(out,"Infrastructure MST: Prim=%.0f, Kruskal=%.0f\n", prim, kruskal);
    if (prim != 33 || kruskal != 33) return 0;
    int weights[] = {2, 3, 4, 5}, values[] = {3, 4, 5, 6}, take[4];
    for (int method = 0; method < 4; method++) {
        counters_reset(&measurement_stats);
        timer_start(&measurement_timer);
        int utility = choose(method, 4, 5, weights, values, take, &measurement_stats);
        const char *orders[] = {"Knapsack_DP", "BB_FIFO", "BB_LIFO", "BB_BestBound"};
        record_algorithm(csv, orders[method], &measurement_timer, &measurement_stats);
        fprintf(out,"Reading plan method %d: optimum=%d\n", method, utility);
        if (utility != 7 || !check_choice(4, 5, weights, values, take, utility)) return 0;
    }
    char subsequence[257];
    counters_reset(&measurement_stats);
    timer_start(&measurement_timer);
    int common_length = lcs("data", "dart", subsequence, sizeof(subsequence), &measurement_stats);
    record_algorithm(csv, "LCS", &measurement_timer, &measurement_stats);
    fprintf(out,"LCS(data,dart) length=%d", common_length);
    fprintf(out,"; subsequence=%s\n", subsequence);
    if (strcmp(subsequence,"dat") != 0) return 0;
    int dims[] = {10, 20, 30, 40, 30};
    counters_reset(&measurement_stats);
    timer_start(&measurement_timer);
    int products = libraa_matrix_chain_order(dims, 4, &measurement_stats);
    record_algorithm(csv, "libraa_matrix_chain_order", &measurement_timer, &measurement_stats);
    fprintf(out,"Report matrix-chain minimum products=%d\n", products);
    if (products != 30000) return 0;
    double hit[] = {.15, .10, .05, .10, .20}, miss[] = {.05, .10, .05, .05, .05, .10};
    int roots[36];
    counters_reset(&measurement_stats);
    timer_start(&measurement_timer);
    double tree_cost = libraa_optimal_bst_roots(hit, miss, 5, roots, &measurement_stats);
    record_algorithm(csv, "libraa_optimal_bst_roots", &measurement_timer, &measurement_stats);
    fprintf(out,"Frequent-keyword optimal BST cost=%.2f\n", tree_cost);
    if (!isfinite(tree_cost) || fabs(tree_cost - 2.75) > 1e-9) return 0;
    print_tree(out, roots, 1, 5, 0, 6);
    double shelves[] = {0, 10, 15, 20, 10, 0, 35, 25, 15, 35, 0, 30, 20, 25, 30, 0};
    int route[5];
    counters_reset(&measurement_stats);
    timer_start(&measurement_timer);
    double cost = libraa_tsp_dp(4, shelves, route, &measurement_stats);
    record_algorithm(csv, "libraa_tsp_dp", &measurement_timer, &measurement_stats);
    if (!isfinite(cost)) return 0;
    fprintf(out,"Exact shelf tour cost=%.0f; route:", cost);
    for (int i = 0; i < 5; ++i) fprintf(out," %d", route[i]);
    fputc('\n', out);
    int board[8];
    counters_reset(&measurement_stats);
    timer_start(&measurement_timer);
    int queens_found = libraa_nqueens(8, board, &measurement_stats);
    record_algorithm(csv, "libraa_nqueens", &measurement_timer, &measurement_stats);
    if (queens_found != 1) return 0;
    fprintf(out,"N-Queens columns by row:");
    for (int i = 0; i < 8; ++i) fprintf(out," %d", board[i]);
    fputc('\n', out);
    int set[] = {10, 7, 5, 18, 12, 20, 15}, subset[7];
    counters_reset(&measurement_stats);
    timer_start(&measurement_timer);
    int subset_found = libraa_sum_of_subsets(set, 7, 35, subset, &measurement_stats);
    record_algorithm(csv, "libraa_sum_of_subsets", &measurement_timer, &measurement_stats);
    if (subset_found != 1) return 0;
    fprintf(out,"Citation target 35 subset:");
    for (int i = 0; i < 7; ++i)if (subset[i]) fprintf(out," %d", set[i]);
    fputc('\n', out);
    int cap[] = {0, 10, 10, 0, 0, 0, 2, 8, 0, 0, 0, 9, 0, 0, 0, 0};
    counters_reset(&measurement_stats);
    timer_start(&measurement_timer);
    int flow = libraa_ford_fulkerson(4, cap, 0, 3, &measurement_stats);
    record_algorithm(csv, "libraa_ford_fulkerson", &measurement_timer, &measurement_stats);
    fprintf(out,"Capacity transfer max flow=%d\n", flow);
    if (flow != 17) return 0;
    int eligible[] = {1, 1, 0, 0, 1, 0, 0, 0, 1}, owner[3];
    counters_reset(&measurement_stats);
    timer_start(&measurement_timer);
    int matches = libraa_max_bipartite_matching(3, 3, eligible, owner, &measurement_stats);
    record_algorithm(csv, "libraa_max_bipartite_matching", &measurement_timer, &measurement_stats);
    fprintf(out,"Copy allocation matching=%d\n", matches);
    if (matches != 3) return 0;
    long long mod[] = {3, 5, 7}, rem[] = {2, 3, 2};
    counters_reset(&measurement_stats);
    timer_start(&measurement_timer);
    long long reconstructed = libraa_chinese_remainder_theorem(mod, rem, 3, &measurement_stats);
    record_algorithm(csv, "libraa_chinese_remainder_theorem", &measurement_timer, &measurement_stats);
    fprintf(out,"CRT ID reconstruction=%lld\n", reconstructed);
    if (reconstructed != 23) return 0;
    for (int method = 0; method < 4; ++method) {
        size_t pos[16];
        counters_reset(&measurement_stats);
        timer_start(&measurement_timer);
        size_t n = match_text("aaaa", "aa", (Matcher)method, pos, 16, &measurement_stats);
        record_algorithm(csv, matcher_name((Matcher)method), &measurement_timer, &measurement_stats);
        if (n != 3 || pos[0] != 0 || pos[1] != 1 || pos[2] != 2) return 0;
        fprintf(out,"%s overlapping positions:", matcher_name((Matcher)method));
        for (size_t i = 0; i < n; ++i) fprintf(out," %zu", pos[i]);
        fputc('\n', out);
    }
    int ids[] = {1, 2, 3, 8};
    int key = 8;
    counters_reset(&measurement_stats);
    timer_start(&measurement_timer);
    int iterative = binary_iter(ids, 4, sizeof(int), &key, compare_ints, &measurement_stats);
    record_algorithm(csv, "Binary_iterative", &measurement_timer, &measurement_stats);
    counters_reset(&measurement_stats);
    timer_start(&measurement_timer);
    int recursive = binary_rec(ids, 4, sizeof(int), &key, compare_ints, &measurement_stats);
    record_algorithm(csv, "Binary_recursive", &measurement_timer, &measurement_stats);
    fprintf(out,"ID 8: iterative index=%d, recursive index=%d\n", iterative, recursive);
    if (iterative != 3 || recursive != 3) return 0;
    int a[] = {3, 1, 2}, b[] = {3, 1, 2};
    counters_reset(&measurement_stats);
    timer_start(&measurement_timer);
    if (!merge_sort(a, 3, sizeof(int), compare_ints, &measurement_stats)) return 0;
    record_algorithm(csv, "Merge_sort", &measurement_timer, &measurement_stats);
    counters_reset(&measurement_stats);
    timer_start(&measurement_timer);
    if (!quick_sort(b, 3, sizeof(int), compare_ints, &measurement_stats)) return 0;
    record_algorithm(csv, "Quick_sort", &measurement_timer, &measurement_stats);
    fprintf(out,"Merge/quick sorts agree=%d\n", memcmp(a, b, sizeof(a)) == 0);
    return same && cost == 80 && memcmp(a, b, sizeof(a)) == 0;
}


int run_lab(FILE *out) {
    return run_examples(out, NULL);
}

static int compare_times(const void *left, const void *right) {
    long long a = *(const long long*)left, b = *(const long long*)right;
    return (a > b)-(a < b);
}

static long long median(long long *times) {
    if (!merge_sort(times, 5, sizeof(*times), compare_times, NULL)) return -1;
    return times[2];
}

int run_scenarios(const Library *lib, FILE *out) {
    if (!lib->count) return 0;
    Hit *hits = malloc((size_t)lib->count*sizeof(Hit)), *baseline = malloc((size_t)lib->count*sizeof(Hit));
    if (!hits || !baseline) {
        free(hits);
        free(baseline);
        return 0;
    }
    const char *query = lib->books[0].keywords[0];
    int expected = search_books(lib, query, NAIVE, baseline, lib->count, NULL), ok = expected >= 0;
    fprintf(out,"1 NORMAL: identical catalog and query '%s'\n", query);
    for (int engine = 0; engine < 2; ++engine) {
        long long times[5];
        Counters c;
        int n = search_books(lib, query, (Matcher)engine, hits, lib->count, NULL);
        for (int r = 0; r < 5; ++r) {
            Timer t;
            counters_reset(&c);
            timer_start(&t);
            n = search_books(lib, query, (Matcher)engine, hits, lib->count, &c);
            times[r] = timer_stop_us(&t);
        }
        int same = n == expected;
        for (int i = 0; same && i < n; ++i) same = hits[i].id == baseline[i].id && hits[i].score == baseline[i].score;
        fprintf(out,"%s: hits=%d, agreement=%d, median_us=%lld, modeled_aux_bytes=%lld\n", matcher_name((Matcher)engine), n, same, median(times), c.aux_bytes);
        ok = ok && same;
    }
    free(hits);
    free(baseline);
    fprintf(out,"2 SHOCK: reading budget falls from 10 to 5 hours\n");
    int n = lib->count < 8?lib->count:8, weights[8], values[8], take[8];
    const char *names[] = {
        "DP","FIFO B&B","LIFO B&B","Best-bound B&B"
    };
    for (int i = 0; i < n; ++i) {
        weights[i] = lib->books[i].hours;
        values[i] = lib->books[i].value;
        fprintf(out,"Book %d: %s, %d hours, value %d\n", lib->books[i].id, lib->books[i].title, weights[i], values[i]);
    }
    /* LIVE EDIT: change these capacities when asked to run another budget. */
    int budgets[] = {10, 5};
    for (int w = 0; w < 2; ++w) {
        int optimum = choose(0, n, budgets[w], weights, values, take, NULL);
        for (int method = 0; method < 4; ++method) {
            Counters c;
            long long times[5];
            int result = choose(method, n, budgets[w], weights, values, take, NULL);
            for (int r = 0; r < 5; ++r) {
                Timer timer;
                counters_reset(&c);
                timer_start(&timer);
                result = choose(method, n, budgets[w], weights, values, take, &c);
                times[r] = timer_stop_us(&timer);
            }
            int valid = check_choice(n, budgets[w], weights, values, take, result) && result == optimum;
            fprintf(out,"W=%d %s: value=%d, feasible/optimal=%d, median_us=%lld, modeled_aux_bytes=%lld; IDs:", budgets[w], names[method], result, valid, median(times), c.aux_bytes);
            for (int i = 0; i < n; ++i)if (take[i]) fprintf(out," %d", lib->books[i].id);
            fputc('\n', out);
            ok = ok && valid;
        }
    }
    int copy_ids[12], count = 0;
    for (int i = 0; i < lib->count && i < 2; ++i)for (int j = 0; j < lib->books[i].copies && j < 6; ++j)copy_ids[count++] = lib->books[i].id;
    fprintf(out,"Copy shock: three requests, up to six CSV copies per first/second title; remove final copy.\n");
    for (int phase = 0; phase < 2; ++phase) {
        int copies = phase && count?count-1:count;
        int edges[36] = {0}, owner[12];
        for (int j = 0; j < copies; ++j) {
            edges[j] = edges[copies+j] = copy_ids[j] == lib->books[0].id;
            edges[2*copies+j] = lib->count > 1 && copy_ids[j] == lib->books[1].id;
        }
        long long mt[5], ft[5];
        int match = 0, flow = 0;
        Counters cm, cf;
        libraa_max_bipartite_matching(3, copies, edges, owner, NULL);
        flow_assignment(3, copies, edges, NULL);
        for (int r = 0; r < 5; ++r) {
            Timer t;
            counters_reset(&cm);
            counters_reset(&cf);
            timer_start(&t);
            match = libraa_max_bipartite_matching(3, copies, edges, owner, &cm);
            mt[r] = timer_stop_us(&t);
            timer_start(&t);
            flow = flow_assignment(3, copies, edges, &cf);
            ft[r] = timer_stop_us(&t);
        }
        fprintf(out,"copies=%d matching=%d flow=%d agreement=%d unmet=%d median_us=%lld/%lld modeled_aux_bytes=%lld/%lld\n", copies, match, flow, match == flow, 3-match, median(mt), median(ft), cm.aux_bytes, cf.aux_bytes);
        ok = ok && match >= 0 && match == flow;
        for (int j = 0; j < copies; ++j)if (owner[j] >= 0) fprintf(out,"reader %d -> copy %d of book %d\n", owner[j], j, copy_ids[j]);
    }
    fprintf(out,"3 STRESS: benchmark runner loads/indexes/searches 100, 1000 and 10000 records.\n");
    return ok;
}

static void bench_row(FILE *f, const char *task, const char *dataset, int n, const char *algorithm, long long *t, const Counters *c, int correct, const char *quality) {
    long long med = median(t);
    fprintf(f,"%s,%s,%d,%s,5,%lld,%lld,%lld,%lld,%lld,%d,%s\n", task, dataset, n, algorithm, med, t[0], t[4], c->comparisons, c->aux_bytes, correct, quality);
}

static unsigned rng_state = 17;
static unsigned next_random(void) {
    rng_state = rng_state*1664525u+1013904223u;
    return rng_state;
}

int run_benchmarks(FILE *out) {
    rng_state = 17;
    if (!make_directory("results") || !make_directory("results/datasets")) return 0;
    FILE *f = fopen("results/benchmarks.csv","w");
    if (!f) return 0;
    int ok = 1;
    fprintf(f,"task,dataset,n,algorithm,repetitions,median_us,min_us,max_us,counted_steps,modeled_aux_bytes,correct,quality\n");
    for (int size = 100; size <= 10000; size *= 10) {
        char path[128], error[256];
        snprintf(path, sizeof(path),"results/datasets/books_%d.csv", size);
        FILE *data = fopen(path,"w");
        if (!data) {
            ok = 0;
            break;
        }
        fprintf(data,"id,title,author,subject,keywords,description,copies,hours,value\n");
        for (int i = 1; i <= size; ++i) fprintf(data,"%d,digital library volume %d,synthetic author,daa,searching;indexing,searching books with algorithms,2,3,5\n", i, i);
        fclose(data);
        Library lib;
        library_init(&lib);
        if (!library_load(&lib, path, error, sizeof(error))) {
            fprintf(out,"%s\n", error);
            ok = 0;
            break;
        }
        Hit *base = malloc((size_t)size*sizeof(Hit)), *hits = malloc((size_t)size*sizeof(Hit));
        int *a = malloc((size_t)size*sizeof(int)), *b = malloc((size_t)size*sizeof(int)), *input = malloc((size_t)size*sizeof(int));
        if (!base || !hits || !a || !b || !input) {
            free(base);
            free(hits);
            free(a);
            free(b);
            free(input);
            library_free(&lib);
            ok = 0;
            break;
        }
        int expected = search_books(&lib,"searching", NAIVE, base, size, NULL);
        for (int e = 0; e < 4; ++e) {
            Counters c;
            long long times[5];
            int good = expected >= 0, count = search_books(&lib,"searching", (Matcher)e, hits, size, NULL);
            for (int r = 0; r < 5; ++r) {
                Timer t;
                counters_reset(&c);
                timer_start(&t);
                count = search_books(&lib,"searching", (Matcher)e, hits, size, &c);
                times[r] = timer_stop_us(&t);
                good = good && count == expected;
                for (int i = 0; good && i < count; ++i) good = hits[i].id == base[i].id && hits[i].score == base[i].score;
            }
            bench_row(f,"catalog_search","full_records", size, matcher_name((Matcher)e), times, &c, good,"same_ranked_results");
            ok = ok && good;
        }
        char *text = malloc((size_t)size+1);
        char pattern[128];
        if (!text) {
            ok = 0;
        }
        else{
            memset(text,'a', (size_t)size);
            text[size] = '\0';
            int len = size/100;
            memset(pattern,'a', (size_t)len);
            pattern[len] = 'b';
            pattern[len+1] = '\0';
            for (int e = 0; e < 4; ++e) {
                Counters c;
                long long times[5];
                int good = 1;
                match_text(text, pattern, (Matcher)e, NULL, 0, NULL);
                for (int r = 0; r < 5; ++r) {
                    Timer t;
                    counters_reset(&c);
                    timer_start(&t);
                    size_t count = match_text(text, pattern, (Matcher)e, NULL, 0, &c);
                    times[r] = timer_stop_us(&t);
                    good = good && count == 0;
                }
                bench_row(f,"matching","repeated_prefix", size, matcher_name((Matcher)e), times, &c, good,"exact_positions");
                ok = ok && good;
            }
            free(text);
        }
        const char *orders[] = {
            "random","sorted","reverse","equal"
        };
        for (int order = 0; order < 4; ++order) {
            for (int i = 0; i < size; ++i)input[i] = order == 0?(int)(next_random()%100000):order == 1?i:order == 2?size-i:7;
            memcpy(b, input, (size_t)size*sizeof(int));
            merge_sort(b, size, sizeof(int), compare_ints, NULL);
            for (int method = 0; method < 2; ++method) {
                Counters c;
                long long times[5];
                int good = 1;
                memcpy(a, input, (size_t)size*sizeof(int));
                if (method)quick_sort(a, size, sizeof(int), compare_ints, NULL);
                else merge_sort(a, size, sizeof(int), compare_ints, NULL);
                for (int r = 0; r < 5; ++r) {
                    memcpy(a, input, (size_t)size*sizeof(int));
                    Timer t;
                    counters_reset(&c);
                    timer_start(&t);
                    if (method)quick_sort(a, size, sizeof(int), compare_ints, &c);
                    else if (!merge_sort(a, size, sizeof(int), compare_ints, &c)) good = 0;
                    times[r] = timer_stop_us(&t);
                    good = good && memcmp(a, b, (size_t)size*sizeof(int)) == 0;
                }
                bench_row(f,"sorting", orders[order], size, method?"Quick":"Merge", times, &c, good,"same_sorted_multiset");
                ok = ok && good;
            }
        }
        for (int i = 0; i < size; ++i)a[i] = i+1;
        for (int method = 0; method < 2; ++method) {
            Counters c;
            long long times[5];
            int good = 1, keys[] = {
                1, size/2, size, size+1
            };
            for (int r = -1; r < 5; ++r) {
                Timer t;
                counters_reset(&c);
                timer_start(&t);
                for (int k = 0; k < 4; ++k) {
                    int at = method?binary_rec(a, size, sizeof(int), &keys[k], compare_ints, &c):binary_iter(a, size, sizeof(int), &keys[k], compare_ints, &c);
                    good = good && at == (keys[k] <= size?keys[k]-1:-1);
                }
                if (r >= 0)times[r] = timer_stop_us(&t);
            }
            bench_row(f,"id_lookup","sorted_ids", size, method?"Recursive":"Iterative", times, &c, good,"exact_id_or_miss");
            ok = ok && good;
        }
        fprintf(out,"Loaded/indexed/searched %d records; equivalent algorithms checked.\n", size);
        free(base);
        free(hits);
        free(a);
        free(b);
        free(input);
        library_free(&lib);
    }
    int weights[] = {2, 3, 4, 5, 1, 6, 3, 2}, values[] = {3, 4, 5, 6, 2, 9, 5, 4}, take[8];
    const char *names[] = {
        "DP","FIFO","LIFO","BestBound"
    };
    for (int W = 10; W >= 5; W -= 5) {
        int optimum = choose(0, 8, W, weights, values, take, NULL);
        char label[32];
        snprintf(label, sizeof(label),"hours_%d", W);
        for (int method = 0; method < 4; ++method) {
            Counters c;
            long long times[5];
            int good = 1;
            choose(method, 8, W, weights, values, take, NULL);
            for (int r = 0; r < 5; ++r) {
                Timer t;
                counters_reset(&c);
                timer_start(&t);
                int result = choose(method, 8, W, weights, values, take, &c);
                times[r] = timer_stop_us(&t);
                good = good && result == optimum && check_choice(8, W, weights, values, take, result);
            }
            bench_row(f,"reading_plan", label, 8, names[method], times, &c, good,"optimal_feasible");
            ok = ok && good;
        }
    }
    if (ferror(f)) ok = 0;
    if (fclose(f)) ok = 0;
    fprintf(out,"Saved results/benchmarks.csv; memory is modeled workspace, not measured process RSS.\n");
    return ok;
}
/* Full evidence runner. The everyday menu never invokes this automatically. */
int run_complete_comparisons(const Library *lib, FILE *out) {
    if (!make_directory("results")) return 0;
    FILE *tests = fopen("results/tests.txt", "w");
    if (!tests) return 0;
    int ok = run_tests(tests);
    if (fclose(tests)) ok = 0;
    fprintf(out, "Regression suite: %s (results/tests.txt)\n", ok ? "PASS" : "FAIL");
    if (!ok) return 0;

    FILE *examples = fopen("results/algorithm_examples.txt", "w");
    FILE *metrics = fopen("results/algorithm_metrics.csv", "w");
    if (!examples || !metrics) {
        if (examples) fclose(examples);
        if (metrics) fclose(metrics);
        return 0;
    }
    fprintf(metrics, "algorithm,example_us,counted_steps,multiplications,modeled_aux_bytes\n");
    ok = run_examples(examples, metrics);
    if (ferror(examples) || ferror(metrics)) ok = 0;
    if (fclose(examples)) ok = 0;
    if (fclose(metrics)) ok = 0;
    fprintf(out, "Required algorithm examples: %s (results/algorithm_examples.txt)\n", ok ? "PASS" : "FAIL");
    if (!ok) return 0;

    FILE *scenarios = fopen("results/scenarios.txt", "w");
    if (!scenarios) return 0;
    ok = run_scenarios(lib, scenarios);
    if (fclose(scenarios)) ok = 0;
    if (!ok) return 0;
    if (!run_scenarios(lib, out) || !run_benchmarks(out)) return 0;
    if (!export_reports(lib, "results")) return 0;
    fprintf(out, "PASS: examples, three scenarios, benchmarks and CSV exports completed.\n");
    return 1;
}
