# Design, requirements and analysis

## Scope

The product remains the simple five-choice CLI. It loads books, searches/ranks, recommends, demonstrates constrained reading/copy allocation, exposes mandatory algorithms and exports CSV. The conversion replaces C++ syntax and containers, not the project brief. The provided evaluation FAQs/guidelines allow a CLI and a simple project with strong understanding, specify an eight-minute demo plus four-minute viva and allow AI assistance when the student understands the code. They do not explicitly waive Project 17's mandatory algorithms.

## Data flow and design

CSV -> validate and normalize -> dynamically allocated Book array -> stable ID sort -> keyword/index entries -> search -> stable relevance ranking -> recommendations -> exports. Load is transactional: the previous catalog is freed/replaced only after a new file is fully valid and indexed. Book IDs are unique, positive and sorted. Keywords are normalized/deduplicated. Reader history and citation edges are small typed fixtures derived from current book IDs, not a complete account system.

IndexEntry stores (keyword, book_id). Entries are merge sorted by word then ID. The shared binary_iter finds an equal word entry; scanning its equal neighbors returns the posting list. Search for arbitrary substrings still scans records, since substring membership is not ordered by ID or keyword sorting.

Search score = 3*title occurrences + 2*keyword occurrences + description occurrences. All ranking and preparation sorts call one generic merge_sort with type-specific comparison rules; the sort logic is not copied per record type. Exact match positions include overlaps. The matcher returns the total count and writes up to the caller's position-buffer capacity. Search ranks small Hit records instead of copying entire Book records.

Recommendation candidates share a keyword or subject. This C implementation scans the catalog to select them, then computes .5*keyword Jaccard + .3*subject equality + .2*LCS/maximum title length. Keyword sets are deduplicated at load. Same formula/output ordering as the simple C++ version; no collaborative-filtering feature. No measured precision/recall or trained ML is claimed. Return up to five recommendations; lower ID breaks equal scores.

## Required scenarios and evidence

1. Normal: identical catalog/query with naive and KMP; compare complete ranked results, time and modeled workspace.
2. Shock: the first up to eight named books under 10 then 5 hours; compare exact DP and FIFO/LIFO/best-bound B&B. Every solution must obey capacity and its selected-book values must sum to the reported optimum. Three requests are assigned compatible physical copies derived from the first two CSV titles (at most six copies per title). Remove the final copy. Matching and unit-capacity source-reader-copy-sink flow must agree; report unmatched requests.
3. Stress: load/index/search actual synthetic 100, 1k and 10k CSV catalogs. Compare all four matchers on identical full records; add difficult repeated-prefix patterns, random/sorted/reverse/equal sorting and binary lookups. Exponential algorithms stay on bounded small instances.

--bench produces 62 comparison rows: 18 rows per dataset size plus 8 optimizer rows. Every comparison uses the same input, one discarded warm-up and five measured runs. Sorting reset/copy and file loading occur outside its timed region. Full-catalog search time includes ranking. Timer uses QueryPerformanceCounter on Windows and CLOCK_MONOTONIC on POSIX. POSIX builds define _POSIX_C_SOURCE through build flags. Operation counters are algorithm-level events, not CPU instruction counts; instrumentation is enabled inside timing. Small runs may measure zero microseconds.

Memory is **modeled auxiliary payload**, not process RSS. Matcher tables and merge buffers use actual element counts. DP records its allocated table bytes. The single B&B engine records its shared reserved frontier payload, even when partly unused. Flow/matching record fixed helper arrays, not total process memory. Persistent catalog/index, result buffers, allocator overhead and most stack frames are excluded. Zero for constant-space algorithms means no modeled dynamic buffer, not literally zero memory use. Data quality means exact-position/rank agreement, same sorted multiset, or feasible exact utility. Reading utility is a chosen input, not a learned relevance estimate.

CSV exports cover catalog, keyword postings, recommendations, reader history and citations. --demo also records the scenario output and benchmarks. Results are regenerated from the current program and environment; old C++ measurements are not carried over.

## Mandatory algorithm map and actual complexity

n=items; t=text bytes; m=pattern bytes; P=total posting entries; V/E=vertices/edges; W=integer hours budget; sigma<=256; B=Huffman payload bits. Space excludes input/output unless stated. Caps do not change the general analysis.

| Algorithm | Library application / C function | Time | Auxiliary space |
|---|---|---|---|
| Merge sort | Book IDs, index, ranking; merge_ints | O(n log n) all cases | O(n) buffers + O(log n) recursion |
| Quick sort | Sorting comparison; quick_ints | O(n log n) balanced/average, O(n^2) worst | O(log n) smaller-side recursion |
| Binary search iterative/recursive | Sorted IDs; binary_iter/rec and find_book | O(1) best, O(log n) average/worst | O(1)/O(log n) |
| Keyword index | Sorted word/ID entries; keyword_lookup | O(log P + matches), after O(P log P) build | Returned IDs excluded |
| Naive | Simple search oracle; match_text | O(t*m) worst, O(t) easy first-char mismatches | O(1) |
| KMP | Default matcher; prefix_table/match_text | O(t+m) worst | O(m) |
| Rabin-Karp | Rolling hash plus character verification | O(t+m) typical with few verifications, O(t*m) worst | O(1) |
| Finite automaton | LPS-based transition table | O(m*sigma+t) including build | O(m*sigma) |
| D&C max-subarray | Peak net borrowing activity | O(n log n) | O(log n) |
| Fast powering | Modular numeric utility | O(log exponent) | O(1) |
| Celebrity | Universally cited document fixture | O(n) elimination + verification | O(1) |
| Classical/Strassen matrix product | Document-keyword similarity blocks | O(n^3)/O(p^log2(7)), p padded power of two | Strassen O(p^2); returned output excluded |
| Activity selection | Maximum non-overlapping reading events | O(n log n) shared merge sort + O(n) scan | O(n) merge buffer |
| Interval partitioning | Minimum rooms for all events | O(n log n) shared sort + O(n^2) room scans | O(n) bounded arrays |
| Huffman | Actual library-text prefix coding | O(t+sigma^2+B), linear minimum-node search | O(sigma^2) code strings/tree; bit output excluded |
| Dijkstra | Nonnegative branch routes | O(V^2) matrix version | O(V) |
| Prim | Branch infrastructure backbone | O(V^2) matrix version | O(V) |
| Kruskal | Independent backbone comparison | O(E log E+EV), shared merge sort and conservative path-compression-only bound | O(E+V) |
| 0/1 knapsack DP | Whole-book hours planner | O(nW), pseudo-polynomial | O(nW) dynamic full table |
| Matrix-chain DP | Cheapest report multiplication order | O(n^3) | O(n^2) bounded table |
| LCS DP | Title similarity | O(t*m) | O(t*m) dynamic table |
| Optimal BST | Frequent-keyword tree experiment | O(n^3) | O(n^2); roots exposed and printed |
| Bellman-Ford | Routes with negative penalties/incentives | O(VE) edge list | O(V) output distances |
| Floyd-Warshall | All-pairs routes | O(V^3) | O(V^2) output distance table |
| Held-Karp TSP | Exact small shelf tour | O(n^2*2^n) | O(n*2^n) dynamic cost/next tables |
| N-Queens | Backtracking validation | O(n!*n) conservative with safety scans | O(n) board/stack |
| Sum of subsets | Exact citation-value target | O(2^n) | O(n) state/stack |
| FIFO/LIFO/best-bound B&B | One libraa_knapsack_01_bb engine with BBOrder | Worst exponential; O(n) bound/witness work per node plus heap work | One shared 8192-node frontier + bounded item arrays |
| Ford-Fulkerson with BFS | Transfer capacity and matching cross-check | O(V^2*A), A augmentations; conservative O(V^3 E) bound for matrix scanning | O(V^2) residual matrix |
| Bipartite matching | Compatible reader-copy assignments | O(L^2 R) conservative matrix DFS bound | O(L+R) state/stack |
| CRT | Reconstruct ID from modular residues | O(k log M) arithmetic steps | O(log M) Euclid stack |

Every preparation sort, including timing medians, calls the same merge_sort. There are no bubble/insertion sort implementations. The two required sort algorithms (merge and quick) remain distinct. The fractional relaxation exists only inside the common B&B upper-bound helper.

## Correctness and tests

- Seeded matcher trials compare complete position arrays with naive, including overlaps/empty/longer patterns.
- Sorts are ordered, agree and preserve the original multiset. Book/index/rank merge logic preserves the ordering invariants.
- Binary tests use a scan oracle for present/missing IDs.
- Max-subarray uses exhaustive intervals; knapsack methods use exhaustive small subsets and validate each witness.
- Fractional relaxation never underestimates the whole-item optimum. B&B never silently discards states: full fixed containers return -1 with errno=E2BIG, not an exact-answer claim.
- Huffman creates actual tree codes and decodes back to input; singleton/four-symbol cases verify payload lengths.
- Padded Strassen agrees with classical at 1..11 matrix sizes, including odd and non-power-of-two dimensions.
- Dijkstra/Bellman-Ford/Floyd agree on the same infinity-encoded graph; zero-weight edges, reachable negative cycles and disconnected MSTs are covered.
- OBST verifies the expected cost, recursively recomputes the returned tree cost, and checks deterministic tie handling. Relative differences <=1e-12 keep the first root; this avoids SSE/x87 platform-dependent optimal ties. TSP verifies complete routes and exact costs, including n=11/12 and an out-of-range rejection.
- Queens/subset witnesses satisfy constraints; matching agrees with independent exhaustive assignment and equivalent unit-capacity max-flow.
- CSV tests cover quoting, exports, invalid copies and transactional replacement; the included sample is a fixed regression fixture.

Tests prove the exercised cases, not universal correctness. Caller-supplied buffers/matrices must satisfy the header contracts. Interfaces return failure (-1, NAN or false depending on return type); void graph/matrix routines signal invalid inputs/allocation failure with errno. Floyd sets errno=EDOM on a negative cycle. Use validated dimensions and check status before interpreting outputs. Allocation is explicit; caller owns output buffers and library_free owns catalog/index memory. No parallel/thread-safe API is claimed for the static B&B buffers.

## Recurrences and production choice

Binary: T(n)=T(n/2)+O(1) -> O(log n), by repeated halving. Merge/D&C max-subarray: 2T(n/2)+O(n) -> O(n log n), O(n) per level over log n levels. Worst quick: T(n-1)+O(n) -> O(n^2) by arithmetic sum; variable split cannot directly use the fixed-fraction Master theorem. Strassen: 7T(n/2)+O(n^2) -> O(n^log2(7)), Master case 1. Power: T(e/2)+O(1) -> O(log e). KMP is nonrecursive: forward movements and pattern fallbacks give an amortized linear bound.

Use KMP as the guaranteed-linear default, naive as oracle/baseline, iterative binary search for IDs, exact-keyword postings for lookup, stable merge ranking, and the explicit LCS/Jaccard heuristic for small catalogs. Naive may beat KMP on easy short text due to constants. Use DP for modest integer budgets and exact tree methods only on small instances; do not run exponential algorithms at 10k items. More advanced indexes/semantic recommendation would be future work, not requirements to inflate the current product.

## Complexity classes and reduction

P: polynomial-time solvable decision problems. NP: yes-certificates verifiable in polynomial time. NP-hard: every NP problem reduces to it. NP-complete: in NP and NP-hard. Literal search is in P.

Map KNAPSACK decision to READING-PLAN decision: item weight -> book hours; item value -> utility; W -> hours budget; V -> required utility. Each item becomes one record. A subset with weight<=W and value>=V exists iff the corresponding book list meets those same inequalities. Construction and certificate checking are polynomial. Thus reading-plan decision is NP-complete and utility maximization is NP-hard. O(nW) DP is pseudo-polynomial because W is encoded in log W bits. The implemented bounded optimizer solves classroom instances; a decision answer follows by comparing optimum to V. The complexity claim concerns the unbounded problem family, not literal NP-completeness of a fixed 20-item function.

## Limits

Catalog <=10k; query<=128 bytes; title<=256, author/subject<=128, description<=2048; at most 32 unique keywords, each<=64 and combined CSV field<=2049. ASCII case folding, whitespace collapse and UTF-8-byte preservation; no Unicode case normalization, multiline CSV or database. Copies 0..100; hours 1..10000; utility 0..1e6. DP<=100 items/W<=10000; B&B<=20 with explicit 8192-container exhaustion failure. TSP<=12 with path buffer n+1; queens<=12; subset<=24 with nonnegative bounded values; matrix multiplication<=128; matrix-chain<=30 matrices/dimensions<=100; OBST<=30; matrix graph helpers<=128; flow and matching sides<=64; Kruskal edges<=512; CRT modulus product<=1e9, pairwise coprime. Integer max-subarray/log examples require sums fitting int. These bounds keep the prototype predictable and must be explained accurately.

## Minimal reusable implementation (c-v1.3)

See REUSE_MAP.md: each named algorithm has one implementation. Removed optional Kadane, standalone fractional/knapsack-BT, copied typed merge sorts, bubble/insertion sorts, copied B&B engines, duplicate search loops and the redundant persistent ID array. Required iterative/recursive binary searches and all other explicitly named alternatives remain. Comparators define ordering only; the shared algorithms handle all arrays through element width and a comparison function. Tiny brute-force routines remain only as the required test oracles.
