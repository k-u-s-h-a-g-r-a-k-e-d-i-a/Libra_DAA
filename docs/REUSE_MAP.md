# The code to explain: one implementation per algorithm

The brief explicitly names alternative algorithms. They remain distinct: merge is not quick, and iterative binary search is not recursive binary search. Repeated uses of any one algorithm call the same function.

| Shared implementation | All callers / uses |
|---|---|
| merge_sort in algorithms.c | Book IDs, keyword entries, search hits, recommendations, activity finish/start ordering, Kruskal edges, B&B ratios, benchmark medians and merge-sort comparisons |
| quick_sort in algorithms.c | Required sort comparison and tests, on integers or records |
| binary_iter / binary_rec in algorithms.c | Book lookup, exact keyword lookup, lab, benchmark and tests |
| prefix_table in algorithms.c | KMP and finite-automaton preprocessing |
| match_text in algorithms.c | Search, lab, benchmarks and tests; four explicitly required engines share the same API |
| lcs in algorithms.c | Recommendation title similarity, lab and tests |
| libraa_fast_pow_mod in workbench.c | Required number-powering example and Rabin-Karp's high-power hash factor |
| libraa_knapsack_01_bb in workbench.c | FIFO, LIFO and best-bound scenarios, benchmarks and tests; only BBOrder changes |
| minimum_unvisited in workbench.c | Required Dijkstra and Prim implementations |

## What a comparator is

A comparator is a small rule, not another sorting algorithm. It receives two records and returns negative if the first belongs before the second, zero if equal, and positive if it belongs after. merge_sort performs the split/merge once, regardless of which rule is supplied.

```c
merge_sort(books, count, sizeof(Book), compare_books, NULL);
merge_sort(hits, hit_count, sizeof(Hit), compare_hits, stats);
```

Same algorithm, different element size and ordering rule. void* means an address without a fixed element type. sizeof tells the algorithm how many bytes to copy per element. Compare is a function-pointer type: it lets the algorithm ask the supplied rule which element comes first. The algorithm copies bytes with memcpy; only the comparator interprets fields. This is ordinary C, not C++ templates.

The binary-search functions use the same idea. Book lookup constructs a key record with the requested ID and calls a shared binary search with compare_books. Keyword lookup searches with compare_words, then scans equal neighboring entries to collect all matching book IDs. There is no separate hand-written keyword binary-search loop or persistent duplicate ID array.

## The shared B&B engine

```c
libraa_knapsack_01_bb(n,W,hours,value,take,BB_FIFO,stats);
libraa_knapsack_01_bb(n,W,hours,value,take,BB_LIFO,stats);
libraa_knapsack_01_bb(n,W,hours,value,take,BB_BEST_BOUND,stats);
```

All three calls use one validation step, one bound function, one include/exclude expansion and one result-update rule. push_node/pop_node implement the required queue, stack and priority ordering. No copied three-version solver body remains. The 8192-node frontier is shared and sequential; failure is explicit. The fractional calculation exists only as an internal optimistic bound.

## What was removed because it is not explicitly requested

- Kadane comparison.
- Standalone fractional-knapsack solver/demo (fractional bound remains necessary for B&B).
- Standalone knapsack-backtracking solver/demo (backtracking is demonstrated by required N-Queens and sum-of-subsets).
- Duplicate typed merge-sort bodies and the separate integer merge-sort implementation.
- Workbench bubble sorts and the extra insertion sort used for five benchmark timings.
- Duplicate Book/keyword binary-search loops and unused wrappers.

Tests still use tiny brute-force oracles. Those are validation evidence explicitly requested by the brief, not extra product features or submitted alternatives to explain as normal app algorithms.
