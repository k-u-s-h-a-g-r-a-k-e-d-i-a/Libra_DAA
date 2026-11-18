# Learn and edit the C version

You do not need to learn C++ for this version. Start with the C syntax already familiar from labs.

## One book

```c
typedef struct {
    int id;
    char title[257];
} Book;
```

`struct` groups fields. `typedef` gives that record the short name Book. `char title[257]` stores at most 256 text bytes plus the final '\0'.

```c
Book b = {0};
b.id = 1;
strcpy(b.title, "Algorithms");
printf("%d %s\n", b.id, b.title);
```

`{0}` clears the record. The dot accesses a field. strcpy copies text into a sufficiently large character array. `%d` prints an int; `%s` prints a C string. Unlike C++, this version does not use std::string or vector.

## One list

```c
Book books[10];
for (int i = 0; i < 10; ++i) {
    printf("%s\n", books[i].title);
}
```

This example assumes the ten records were filled first. The actual catalog uses malloc/realloc because its record count can grow to 10,000. That is still a contiguous array of Book records. `lib->count` tells us how many are valid.

## Pointers used by this project

```c
const Book *b = find_book(&lib, 1, 0);
if (b != NULL) {
    printf("%s\n", b->title);
}
```

- `&lib`: address of the Library record; lets the function read it without copying everything.
- `Book *`: address of a Book record.
- `NULL`: no record found.
- `b->title`: the title of the record pointed to by b. Equivalent to `(*b).title`.
- `const`: the function/caller must not modify the record through this pointer.
- `find_book(...,0)`: iterative search; `...,1` selects recursive search.

`malloc(count * sizeof(Book))` reserves room. Check the result against NULL. `free(pointer)` releases owned memory. Do not free the result of find_book: it points into the catalog, which library_free owns. Headers declare functions; .c files implement them.

## How to follow the code

1. main.c: find `main` and the `choice == 1` branch.
2. library.h: inspect Book, Hit and Library.
3. library.c: inspect `search_books`, then `rank_hits` / `merge_hits`.
4. algorithms.c: read the NAIVE part of `match_text` before KMP.
5. library.c: inspect `recommend_books`.
6. Then study KMP's prefix_table and the LCS table.

main collects inputs; search_books does the work. A Hit contains only book ID and score, so ranking does not need to copy whole book descriptions.

## A simple search dry run

Title contains “sorting” once and a keyword is also “sorting”. Score=3*1 + 2*1=5, plus any description occurrences. Larger score ranks first; ties use lower ID.

Naive tries every possible starting position. KMP uses an LPS table to reuse matched prefixes. “aa” in “aaaa” matches at 0,1,2, including overlaps.

## Live edits to practice before the viva

Use Ctrl+F for the marker **LIVE EDIT**. Save, rebuild and run the same input again.

| Request | File/function | Small edit |
|---|---|---|
| Give title matches more weight | library.c / search_books | Change `3.0` to `5.0` |
| Give keywords more recommendation weight | library.c / recommend_books | Change `.5`, `.3`, `.2`; choose weights summing to 1 for an interpretable scale |
| Try a different hours constraint | evidence.c / run_scenarios | Change `budgets[] = {10,5}` to `{8,4}` |
| Try another text/pattern | evidence.c / run_lab | Change “aaaa” and “aa” in its matcher example |
| Change a book's hours or available copies | data/books.csv | Change hours/copies; keep all nine columns valid |
| Try another exact ID | Running menu | Enter a present ID and a missing ID |

After score changes, do not expect old benchmark numbers to remain valid. Rerun --test and --demo. Tests about the fixed sample data may need adjustment if you deliberately change its IDs/keywords; never delete a test just to hide a bug.

## Preparation order

First session: records, arrays, pointers, function arguments and the search score. Explain one book and one loop without reading notes.

Second session: naive matching, merge-sort ranking, binary search and the keyword index. Dry-run each on tiny inputs.

Third session: KMP LPS, LCS, the recommendation formula, time and space complexity. Trace one recommendation manually.

Fourth session: reading DP versus BT/B&B, matching/flow shock and stress results. Know why the same feasible problem should have the same optimum.

Finally: use the mandatory map in docs/DESIGN_AND_ANALYSIS.md for remaining algorithms, recurrences, P/NP and the reading-plan reduction. They remain examinable; separating the lab does not waive them. Practice changing at least three small things and rebuilding successfully before presenting.

## Eight-minute demonstration

- 0:00-0:40: problem and CSV record.
- 0:40-2:20: search “sorting” with naive and KMP; explain equal results and one score.
- 2:20-3:20: recommend for ID 1; explain keywords, subject and LCS.
- 3:20-5:10: normal and capacity-shock comparisons in menu 3.
- 5:10-6:15: benchmark CSV at 100/1k/10k: correctness, time, modeled memory and quality.
- 6:15-7:15: show search_books and run a new query; be ready for a small requested edit.
- 7:15-8:00: show exports and lab availability; state limitations.

The four-minute viva can cover the whole project and DAA foundations. Understanding a small implementation helps; memorizing a paragraph is not a substitute for tracing its code.
