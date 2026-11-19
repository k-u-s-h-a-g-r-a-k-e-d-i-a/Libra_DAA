# LibraDAA — Digital Library Search and Recommendation Engine

A portable, offline **C11** command-line implementation of DAA Project 17. This version retains the simple application's features and replaces the C++ code with C structs, arrays, strings and functions.

## Features

- Literal substring search with naive, KMP, Rabin-Karp and finite-automaton matching.
- Iterative and recursive binary search for book IDs.
- Sorted keyword/posting index for exact keyword retrieval.
- Stable merge-sort ranking and an explainable keyword/subject/LCS recommendation formula.
- Reading-hour optimization and physical-copy assignment scenarios.
- Mandatory syllabus algorithm lab, correctness tests and reproducible benchmarks.
- CSV export of catalog, index, recommendations and reading/citation relationships.

## Build and run

Extract into a fresh folder, then open **the folder containing run.bat** in Antigravity. Do not mix it with the earlier C++ folder.

Windows PowerShell:

```
.\run.bat
```

Linux:

```
make run
```

A C11 compiler is required. The Windows script uses **gcc**, supplied with ordinary MinGW installations; no C++ compiler or C++17 headers are required. The previous MinGW GCC 6.3 toolchain is the compatibility target; actual Windows execution must still be checked on the user's laptop. Tested here with GCC 14 on Linux.

In Antigravity, the included build/run tasks compile all C files together. “Run current file” on main.c is insufficient. Debugging is optional and needs a compatible debugger extension and gdb; the extension's `cppdbg` name also covers C programs.

Direct Linux compilation:

```
gcc -std=c11 -O2 -Wall -Wextra -Wpedantic -D_POSIX_C_SOURCE=200809L src/main.c src/library.c src/algorithms.c src/workbench.c src/evidence.c src/tests.c -lm -o libradaa
```

## Menu and commands

1. Search books: substring, exact ID or keyword index.
2. Recommend books.
3. Run required scenarios and benchmarks.
4. Show the algorithm lab.
5. Load or export CSV.

0 exits.

```
./libradaa --test
./libradaa --lab
./libradaa --bench
./libradaa --demo
```

On Windows replace `./libradaa` with `.\libradaa.exe`.

## Structure

| File | Responsibility |
|---|---|
| src/main.c | Input, five-choice menu, printed output |
| src/library.h and library.c | Records, CSV validation, index, search, recommendations, export |
| src/algorithms.h and algorithms.c | Matchers, binary searches, shared sorts and LCS |
| src/workbench.h and workbench.c | Remaining mandatory syllabus algorithms |
| src/counters.h | Operation counters, modeled memory and platform timer |
| src/evidence.c | Domain examples, three scenarios and benchmark CSV |
| src/tests.c | Independent small-instance oracles and regressions |
| data/books.csv | Ten-book sample; same nine-column schema as the simple C++ version |
| docs/REUSE_MAP.md | One implementation per algorithm, shared callers and comparator explanation |
| docs/DESIGN_AND_ANALYSIS.md | Requirements, complexity, correctness, limitations and reduction |
| my_reference/ | Personal learning and live-edit notes; not application dependencies |

CSV header: `id,title,author,subject,keywords,description,copies,hours,value`. Keywords use semicolons. Quoted commas/doubled quotes are supported; multiline fields are not. Bad rows, duplicate IDs and limit violations reject the whole load while preserving the old catalog.

`--demo` writes scenario/benchmark and CSV reports to results/. `--bench` generates 100/1k/10k synthetic catalogs in results/datasets/ and compares identical inputs. Memory columns report modeled auxiliary payload, not measured process RSS. Tiny runs may measure 0 microseconds; larger runs are more informative. Synthetic recommendation scores are not measured human relevance accuracy.

Optional CMake configuration builds a reusable static `daa` library and the CLI; CMake was not available for execution in the verification environment. Use the tested Make/direct-GCC route or Windows batch script.

## Provenance

C conversion with AI assistance, reusing and repairing the earlier C workbench and retaining the simple version's product behavior. Honest personal contribution details should be supplied by the student after reviewing and modifying the code. The local release tag is `c-v1.4`; no remote repository or eight-week development history is claimed. Standard C runtime I/O, strings, allocation and math support the explicitly implemented algorithms. No library sort, text-search or graph solver replaces assessed algorithms.

## Windows regression fix (c-v1.4)

The test source explicitly includes time.h for clock(). OBST near-equal floating-point ties retain the first root within relative tolerance 1e-12. Tests verify expected optimal cost, returned-tree cost, and deterministic tie handling. The five-key fixture has two mathematical optimal roots (2 and 4); the former test was too specific without a tie contract. The corrected suite passes native SSE and x87-style evaluation builds.

Each algorithm is implemented once and reused across the app, lab, tests and benchmarks. See docs/REUSE_MAP.md for every call site. Optional standalone fractional/knapsack-BT, copied record sorts, bubble/insertion sorts and copied B&B engines are removed. Both ID-search styles, four required text matchers, merge/quick algorithms and mandatory graph/optimization variants remain.
