#include "library.h"
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

int run_lab(FILE *out) {
    fprintf(out,"ALGORITHM LAB: required analytical examples, not extra app features\n");
    int activity[] = {-2, 1, -3, 4, -1, 2, 1, -5, 4}, first, last;
    int peak = libraa_max_subarray_dc(activity, 0, 8, &first, &last, NULL);
    fprintf(out,"Peak borrowing window, divide and conquer=%d\n", peak);
    if (peak != 6) return 0;
    long long power = libraa_fast_pow_mod(2, 10, 1000, NULL);
    fprintf(out,"Fast power: 2^10 mod 1000=%lld\n", power);
    if (power != 24) return 0;
    int citations[] = {0, 1, 1, 0, 0, 1, 0, 0, 0};
    int celebrity = libraa_celebrity_find(citations, 3, NULL);
    fprintf(out,"Universally cited document (zero-based)=%d\n", celebrity);
    if (celebrity != 2) return 0;
    double A[] = {1, 2, 3, 4}, B[] = {5, 6, 7, 8}, C[4], S[4];
    Counters cc, cs;
    counters_reset(&cc);
    counters_reset(&cs);
    errno = 0;
    libraa_matrix_multiply_classical(2, A, B, C, &cc);
    libraa_matrix_multiply_strassen(2, A, B, S, &cs);
    if (errno == ENOMEM) return 0;
    int same = memcmp(C, S, sizeof(C)) == 0;
    fprintf(out,"Similarity matrix products agree=%d; scalar multiplications classical=%lld, Strassen=%lld\n", same, cc.multiplications, cs.multiplications);
    Activity slots[] = {
        {1, 1, 4}, {2, 2, 5}, {3, 4, 7}, {4, 3, 6}
    };
    int selected[4], rooms[4];
    int scheduled = libraa_activity_selection(slots, 4, selected, NULL), room_count = libraa_interval_partitioning(slots, 4, rooms, NULL);
    fprintf(out,"Reading-room events: %d non-overlapping selections; %d rooms to hold all\n", scheduled, room_count);
    if (scheduled != 2 || room_count != 3) return 0;
    HuffmanTree tree;
    char bits[8192], decoded[2049];
    const char *text = "algorithms and digital library";
    int bit_count = libraa_huffman_encode(text, bits, sizeof(bits), &tree, NULL);
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
    libraa_dijkstra(6, 0, g, distances, parent, NULL);
    libraa_bellman_ford(6, edges, u, v, edge_weights, 0, bf, NULL);
    libraa_floyd_warshall(6, g, all, NULL);
    fprintf(out,"Branch route 0->5: Dijkstra=%.0f, Bellman-Ford=%.0f, Floyd=%.0f\n", distances[5], bf[5], all[5]);
    if (distances[5] != 11 || bf[5] != 11 || all[5] != 11) return 0;
    edges = 0;
    for (int i = 0; i < 6; ++i)for (int j = i+1; j < 6; ++j)if (isfinite(g[i*6+j])) {
        u[edges] = i;
        v[edges] = j;
        edge_weights[edges++] = g[i*6+j];
    }
    double prim = libraa_prim_mst(6, g, parent, NULL);
    double kruskal = libraa_kruskal_mst(6, edges, u, v, edge_weights, NULL);
    fprintf(out,"Infrastructure MST: Prim=%.0f, Kruskal=%.0f\n", prim, kruskal);
    if (prim != 33 || kruskal != 33) return 0;
    int weights[] = {2, 3, 4, 5}, values[] = {3, 4, 5, 6}, take[4];
    for (int method = 0; method < 4; method++) {
        int utility = choose(method, 4, 5, weights, values, take, NULL);
        fprintf(out,"Reading plan method %d: optimum=%d\n", method, utility);
        if (utility != 7 || !check_choice(4, 5, weights, values, take, utility)) return 0;
    }
    char subsequence[257];
    fprintf(out,"LCS(data,dart) length=%d", lcs("data","dart", subsequence, sizeof(subsequence), NULL));
    fprintf(out,"; subsequence=%s\n", subsequence);
    if (strcmp(subsequence,"dat") != 0) return 0;
    int dims[] = {10, 20, 30, 40, 30};
    int products = libraa_matrix_chain_order(dims, 4, NULL);
    fprintf(out,"Report matrix-chain minimum products=%d\n", products);
    if (products != 30000) return 0;
    double hit[] = {.15, .10, .05, .10, .20}, miss[] = {.05, .10, .05, .05, .05, .10};
    int roots[36];
    double tree_cost = libraa_optimal_bst_roots(hit, miss, 5, roots, NULL);
    fprintf(out,"Frequent-keyword optimal BST cost=%.2f\n", tree_cost);
    if (tree_cost != 2.75) return 0;
    print_tree(out, roots, 1, 5, 0, 6);
    double shelves[] = {0, 10, 15, 20, 10, 0, 35, 25, 15, 35, 0, 30, 20, 25, 30, 0};
    int route[5];
    double cost = libraa_tsp_dp(4, shelves, route, NULL);
    if (!isfinite(cost)) return 0;
    fprintf(out,"Exact shelf tour cost=%.0f; route:", cost);
    for (int i = 0; i < 5; ++i) fprintf(out," %d", route[i]);
    fputc('\n', out);
    int board[8];
    if (libraa_nqueens(8, board, NULL) != 1) return 0;
    fprintf(out,"N-Queens columns by row:");
    for (int i = 0; i < 8; ++i) fprintf(out," %d", board[i]);
    fputc('\n', out);
    int set[] = {10, 7, 5, 18, 12, 20, 15}, subset[7];
    if (libraa_sum_of_subsets(set, 7, 35, subset, NULL) != 1) return 0;
    fprintf(out,"Citation target 35 subset:");
    for (int i = 0; i < 7; ++i)if (subset[i]) fprintf(out," %d", set[i]);
    fputc('\n', out);
    int cap[] = {0, 10, 10, 0, 0, 0, 2, 8, 0, 0, 0, 9, 0, 0, 0, 0};
    int flow = libraa_ford_fulkerson(4, cap, 0, 3, NULL);
    fprintf(out,"Capacity transfer max flow=%d\n", flow);
    if (flow != 17) return 0;
    int eligible[] = {1, 1, 0, 0, 1, 0, 0, 0, 1}, owner[3];
    int matches = libraa_max_bipartite_matching(3, 3, eligible, owner, NULL);
    fprintf(out,"Copy allocation matching=%d\n", matches);
    if (matches != 3) return 0;
    long long mod[] = {3, 5, 7}, rem[] = {2, 3, 2};
    long long reconstructed = libraa_chinese_remainder_theorem(mod, rem, 3, NULL);
    fprintf(out,"CRT ID reconstruction=%lld\n", reconstructed);
    if (reconstructed != 23) return 0;
    for (int method = 0; method < 4; ++method) {
        size_t pos[16], n = match_text("aaaa","aa", (Matcher)method, pos, 16, NULL);
        if (n != 3 || pos[0] != 0 || pos[1] != 1 || pos[2] != 2) return 0;
        fprintf(out,"%s overlapping positions:", matcher_name((Matcher)method));
        for (size_t i = 0; i < n; ++i) fprintf(out," %zu", pos[i]);
        fputc('\n', out);
    }
    int ids[] = {1, 2, 3, 8};
    int key = 8;
    int iterative = binary_iter(ids, 4, sizeof(int), &key, compare_ints, NULL);
    int recursive = binary_rec(ids, 4, sizeof(int), &key, compare_ints, NULL);
    fprintf(out,"ID 8: iterative index=%d, recursive index=%d\n", iterative, recursive);
    if (iterative != 3 || recursive != 3) return 0;
    int a[] = {3, 1, 2}, b[] = {3, 1, 2};
    if (!merge_sort(a, 3, sizeof(int), compare_ints, NULL)) return 0;
    if (!quick_sort(b, 3, sizeof(int), compare_ints, NULL)) return 0;
    fprintf(out,"Merge/quick sorts agree=%d\n", memcmp(a, b, sizeof(a)) == 0);
    return same && cost == 80 && memcmp(a, b, sizeof(a)) == 0;
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
