# LibraDAA Complete

A C library-search application with a small everyday menu and a separate executable comparison runner. All algorithms named in the supplied Project 17 brief are implemented. Each algorithm has one implementation shared by the product, comparisons and tests.

## Run the application

Extract into a fresh folder and open the folder containing run.bat. In PowerShell run `./run.bat`. GCC must be available in PATH. The script changes to its own folder, compiles the four C files and starts the menu.

| Menu | What you do | Algorithm used |
|---|---|---|
| 1 Show books | See IDs and resource fields | Array traversal |
| 2 Search text | Enter sorting | KMP, weighted score, merge-sort ranking |
| 3 Find book by ID | Enter 7 | Iterative binary search |
| 4 Recommend similar books | Enter 1 | Jaccard, subject equality, title LCS, merge sort |
| 5 Build a reading plan | Enter 10 or 5 hours | 0/1 knapsack DP |
| 6 Exact keyword lookup | Enter sorting | Binary search in the sorted posting index |
| 0 Exit | Close the application | Memory cleanup |

Normal features use one chosen strategy. The alternative implementations run through the command below.

## Run the required comparisons

Exit the menu first, then run `./run.bat --compare` in PowerShell. This command builds the same executable and runs the verification suite, every required algorithm example, all three scenarios, benchmarks and CSV exports. It ends with PASS only when all stages succeed. The runner stops and reports failure if a stage fails.

Other commands are `./run.bat --test` and `./run.bat --export`. The first runs the regression suite, and the second exports the current sample catalog, postings, recommendations and relationships.

On Linux use `make run`, `make compare` or `make test`. `make` builds both the executable and the reusable static algorithm archive libdaa.a. C11 and the standard C library are required. No third-party algorithm solver is used.

## Only five source files to navigate

| File | Responsibility | How to study |
|---|---|---|
| src/main.c | Inputs, six menu choices and the reading-plan call | Trace one menu choice |
| src/library.c | CSV loading, index, search, recommendation and exports | Start with search_books and recommend_books |
| src/algorithms.c | Every algorithm, each implemented once | Read the algorithm you demonstrate |
| src/verification.c | Tests, mandatory examples, scenarios and measurement | Understand how answers are checked |
| src/libra.h | Shared records, declarations, timer and LIVE EDIT settings | Book and Hit first, other declarations as needed |

A helper function handles one part of an algorithm or a caller handles input/output. Their presence does not indicate another implementation of the same algorithm. Comparators define the ordering of records. The single merge_sort uses those rules for books, postings, hits, schedules, graph edges and timing medians.

The code is consolidated from the full C edition. Reducing file count makes navigation simpler. Keeping the mandatory implementations means the full algorithm module is still substantial.

## Expected product results and live edits

Search sorting returns IDs 1,2,10 with relevance scores 3,2,2. Book 1 has one keyword occurrence and one description occurrence. The formula is TITLE_WEIGHT times title matches plus KEYWORD_WEIGHT times keyword matches plus DESCRIPTION_WEIGHT times description matches. Matching includes overlaps and ignores ASCII capitalization. Equal scores use lower book ID.

Recommendation for ID 1 puts Text Algorithms first with similarity 0.650. Candidates share a keyword or subject. The score combines keyword Jaccard, subject equality and normalized title LCS with weights 0.5,0.3,0.2. Jaccard is intersection size divided by union size. LCS is the longest matching character subsequence in order, allowing gaps. A similarity score is a hand-chosen heuristic rather than a measured relevance probability.

Find LIVE EDIT near the top of src/libra.h. Double DESCRIPTION_WEIGHT from 1.0 to 2.0 and predict book 1's sorting score rising from 3 to 4. Change RECOMMENDATION_LIMIT from 5 to 3 to show three recommendations. You can also adjust the similarity weights. Rebuild after an edit. You may enter another reading budget directly through menu 5.

The ordinary reading planner uses all ten sample books. At ten hours it obtains utility 25 using IDs 2,6,7,9. At five hours it obtains utility 14 using IDs 7,9. The comparison shock intentionally uses the FIRST EIGHT books, so its corresponding optimal values are 24 and 12. These are different inputs. Keep that distinction clear when explaining the results.

## Evidence written by --compare

| Output | Meaning |
|---|---|
| results/tests.txt | 5502 assertions, independent small oracles, witnesses and regression checks |
| results/algorithm_examples.txt | Every mandatory algorithm executed on its analytical example, with result checks |
| results/algorithm_metrics.csv | 34 instrumented example executions, including all three B&B orders |
| results/scenarios.txt | Normal search agreement, reading/copy capacity shock and stress description |
| results/benchmarks.csv | 62 same-input comparison rows with warm-up, five runs, min/median/max and correctness |
| results/catalog.csv | Normalized catalog export |
| results/index.csv | Keyword posting export |
| results/recommendations.csv | Recommendations for the first book |
| results/relationships.csv | Typed reading-history and citation fixtures |
| results/datasets/ | Generated 100,1000,10000-book CSV catalogs, recreated when running comparisons |

The generated stress catalogs are actually loaded, indexed and searched. Four matchers compare complete ranked results. Random, sorted, reverse and equal sort inputs plus repeated-prefix text exercise difficult cases. Exact exponential solvers stay on bounded small inputs.

The normal scenario compares naive and KMP. The shock compares DP and FIFO/LIFO/best-bound B&B on identical first-eight-book inputs at two budgets. It also compares bipartite matching with equivalent unit-capacity max-flow before and after removing a copy. Matching and flow agree on three then two satisfied requests. The large-data scenario uses the benchmark runner.

The analytical examples additionally compare classical and Strassen products, Dijkstra/Bellman-Ford/Floyd shortest paths on one graph and Prim/Kruskal MSTs on one graph. Dedicated tests verify Huffman round trips, OBST tree cost, TSP route witnesses, queens/subset constraints, CRT results and the other required examples. The metric CSV reports executions rather than claiming 34 independent algorithm families.

## Measurement limits

algorithm_metrics.csv contains single small-example timings in microseconds. These timings are illustrative and can be zero at the timer's resolution. benchmarks.csv uses a discarded warm-up and five repetitions for the timed workloads. Compare strategies on identical input within the same environment. Timing includes instrumentation overhead.

Memory is modeled auxiliary payload, not measured process RSS. Reported numbers track explicitly instrumented buffers and exclude persistent catalog/index, inputs/outputs, allocator overhead and most stack frames. Some analytical routines have no complete memory instrumentation, so a zero field can mean no workspace was recorded. Use the theoretical space table for their complexity. All memory columns are estimates with those limits, not full process measurements.

The Windows OBST lab failure is fixed. Its expected cost is checked with a finite-value test and tolerance 1e-9 rather than exact floating-point equality. The OBST implementation also keeps deterministic near-equal root ties.

## Requirements and algorithm map

| Group | Implementations available through comparisons |
|---|---|
| Sorting/searching | Merge sort, quick sort, iterative and recursive binary search, posting index |
| Text matching | Naive, KMP, Rabin-Karp, finite automaton |
| Divide and conquer | Maximum subarray, fast modular powering, celebrity, classical/Strassen multiplication, sorts/searches above |
| Greedy | Activity selection, interval partitioning, Huffman, Dijkstra, Prim, Kruskal |
| Dynamic programming | 0/1 knapsack, matrix chains, LCS, OBST, Bellman-Ford, Floyd-Warshall, Held-Karp TSP |
| Backtracking | N-Queens and sum of subsets |
| Branch and bound | One exact knapsack engine with FIFO, LIFO and best-bound frontier orders |
| Capacity/assignment | Ford-Fulkerson using BFS augmenting paths and maximum bipartite matching |
| Number theory | CRT with modular inverses from extended Euclid |

Records represent books, keywords, readers, histories and citations. Reader and citation records are small derived fixtures, and graph/scheduling examples are meaningful analytical cases. They do not constitute a complete account, loan or branch-management system. The comparison runner implements and executes the mandatory algorithms without putting every algorithm into everyday book search.

The package includes Git development history and release tag complete-v1.0. Earlier history records AI-assisted development under Codex. No remote repository or eight-week history is claimed. Supply your own accurate contribution explanation. Source history has been preserved rather than attributed to a different person.

## Eight-minute live demonstration

1. First minute, explain one Book record and show the sample CSV.
2. Next two minutes, run search sorting and explain KMP plus the scoring line. Show that merge_sort ranks the hits.
3. Next minute, use ID 7 and exact keyword sorting. Explain the sorted-data requirement.
4. Next minute, recommend for ID 1 and explain one similarity calculation.
5. Next minute, show a ten-hour reading plan and reduce it to five hours.
6. Final two minutes, run --compare and open the scenario and benchmark reports. Explain one same-input comparison, the capacity shock and the stress correctness flag.

You can generate reports before the demo, then rerun the command if asked. Read the purpose, logic, time/space complexity and alternatives for algorithms you mention. The viva can cover the full submitted project. The original presentation uses older menu numbers. Use this README's sequence for this consolidated edition.

## Validation

The release is checked through the regular menu and --compare. Verification includes a native strict C11 build, the 5502-assertion suite, all required examples, stress comparisons and exports. The x87-style comparison run specifically checks that execution continues beyond OBST to TSP, matching, CRT and final PASS. AddressSanitizer and UndefinedBehaviorSanitizer run against the complete comparison path with leak detection disabled due to the runtime restriction. Actual Windows/Antigravity execution still needs checking on the evaluation laptop.

## Complexity, correctness and limits reference

The following reference describes the retained implementations. n means item count, V/E graph vertices/edges, W an integer capacity, and string lengths are stated per row.

## Mandatory algorithm map and actual complexity

n=items; t=text bytes; m=pattern bytes; P=total posting entries; V/E=vertices/edges; W=integer hours budget; sigma<=256; B=Huffman payload bits. Space excludes input/output unless stated. Caps do not change the general analysis.

| Algorithm | Library application / C function | Time | Auxiliary space |
|---|---|---|---|
| Merge sort | Book IDs, index, ranking; merge_sort | O(n log n) all cases | O(n) buffers + O(log n) recursion |
| Quick sort | Sorting comparison; quick_sort | O(n log n) balanced/average, O(n^2) worst | O(log n) smaller-side recursion |
| Binary search iterative/recursive | Sorted IDs; binary_iter/rec and find_book | O(1) best, O(log n) average/worst | O(1)/O(log n) |
| Keyword index | Sorted word/ID entries; keyword_lookup | O(log P + matches), after O(P log P) build | Returned IDs excluded |
| Naive | Simple search oracle; match_text | O(t*m) worst, O(t) easy first-char mismatches | O(1) |
| KMP | Recommended matcher; prefix_table/match_text | O(t+m) worst | O(m) |
| Rabin-Karp | Rolling hash plus character verification | O(t+m) typical with few verifications, O(t*m) worst | O(1) |
| Finite automaton | LPS-based transition table | O(m*sigma+t) including build | O(m*sigma) |
| D&C max-subarray | Peak net borrowing activity | O(n log n) | O(log n) |
| Fast powering | Modular numeric utility | O(log exponent) | O(1) |
| Celebrity | Universally cited document fixture | O(n) elimination + verification | O(log n) recursive stack |
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
