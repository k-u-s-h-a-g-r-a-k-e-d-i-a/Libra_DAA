#include "libra.h"
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif
static int read_line(const char *prompt, char *out, size_t capacity) {
    for (;;) {
        printf("%s", prompt);
        if (!fgets(out, (int)capacity, stdin)) return 0;
        if (!strchr(out, '\n')) {
            int next = getchar();
            if (next != '\n' && next != EOF) {
                while (next != '\n' && next != EOF) next = getchar();
                puts("Input too long. Try again.");
                continue;
            }
        }
        out[strcspn(out, "\r\n")] = '\0';
        return 1;
    }
}

static int read_number(const char *prompt, int *out) {
    char line[64], *end;
    long value;
    if (!read_line(prompt, line, sizeof(line))) return 0;
    errno = 0;
    value = strtol(line, &end, 10);
    if (errno || end == line || *end || value < 0 || value > INT_MAX) {
        puts("Enter a nonnegative whole number.");
        *out = -1;
        return 1;
    }
    *out = (int)value;
    return 1;
}

static void print_hits(const Library *lib, const Hit *hits, int n) {
    if (n < 0) {
        puts("Invalid input or insufficient memory.");
        return;
    }
    if (!n) puts("No results.");
    for (int i = 0; i < n; ++i) {
        const Book *b = find_book(lib, hits[i].id, 0);
        printf("ID %d | %s | score %.3f\n", hits[i].id, b->title, hits[i].score);
    }
}

static void locate_data(const char *program) {
    FILE *f = fopen("data/books.csv","r");
    if (f) {
        fclose(f);
        return;
    }
    char path[1024];
    if (strlen(program) >= sizeof(path)) return;
    strcpy(path, program);
    char *slash = strrchr(path,'/'), *back = strrchr(path,'\\');
    if (back && (!slash || back > slash)) slash = back;
    if (!slash) return;
    *slash = '\0';
#ifdef _WIN32
    if (_chdir(path) != 0) return;
#else
    if (chdir(path) != 0) return;
#endif
    f = fopen("data/books.csv","r");
    if (f) {
        fclose(f);
        return;
    }
#ifdef _WIN32
    (void)_chdir("..");
#else
    (void)chdir("..");
#endif
}

/* The regular menu uses one strategy for each task. Required alternatives run with --compare. */
static void show_books(const Library *lib) {
    puts("ID | Title | Subject | Copies | Hours | Value");
    for (int i = 0; i < lib->count; i++) {
        const Book *book = &lib->books[i];
        printf("%d | %s | %s | %d | %d | %d\n", book->id, book->title, book->subject,
               book->copies, book->hours, book->value);
    }
}

static void search_menu(const Library *lib) {
    char query[129];
    if (!read_line("Search word or phrase: ", query, sizeof(query))) return;
    Hit *results = malloc((size_t)lib->count * sizeof(Hit));
    if (!results) { puts("Not enough memory."); return; }
    /* KMP finds occurrences. search_books calculates scores and calls merge_sort. */
    int count = search_books(lib, query, KMP, results, lib->count, NULL);
    print_hits(lib, results, count);
    printf("Relevance weights: title %.1f, keywords %.1f, description %.1f\n",
           TITLE_WEIGHT, KEYWORD_WEIGHT, DESCRIPTION_WEIGHT);
    free(results);
}

static void find_menu(const Library *lib) {
    int id;
    if (!read_number("Book ID: ", &id)) return;
    const Book *book = find_book(lib, id, 0);
    if (!book) { puts("Book ID not found. Choose Show books to see available IDs."); return; }
    printf("%s by %s\nSubject %s | Copies %d | Hours %d | Value %d\n",
           book->title, book->author, book->subject, book->copies, book->hours, book->value);
}

static void recommend_menu(const Library *lib) {
    int id;
    if (!read_number("Recommend books similar to book ID: ", &id)) return;
    if (!find_book(lib, id, 0)) { puts("Book ID not found. Choose Show books to see available IDs."); return; }
    Hit *results = malloc((size_t)lib->count * sizeof(Hit));
    if (!results) { puts("Not enough memory."); return; }
    int count = recommend_books(lib, id, results, lib->count);
    print_hits(lib, results, count);
    free(results);
}

static void reading_menu(const Library *lib) {
    if (lib->count > 100) { puts("The exact reading planner supports up to 100 books."); return; }
    int budget;
    if (!read_number("Available reading hours (0 to 10000): ", &budget)) return;
    if (budget < 0 || budget > 10000) { puts("Choose a budget from 0 to 10000."); return; }
    int hours[100], values[100], chosen[100];
    for (int i = 0; i < lib->count; i++) {
        hours[i] = lib->books[i].hours;
        values[i] = lib->books[i].value;
    }
    int best = libraa_knapsack_01_dp(lib->count, budget, hours, values, chosen, NULL);
    if (best < 0) { puts("The reading plan could not be calculated."); return; }
    printf("Best total value: %d\n", best);
    int used = 0;
    for (int i = 0; i < lib->count; i++) {
        if (chosen[i]) {
            const Book *book = &lib->books[i];
            printf("ID %d | %s | %d hours | value %d\n", book->id, book->title, book->hours, book->value);
            used += book->hours;
        }
    }
    printf("Hours used: %d of %d\n", used, budget);
}

static void keyword_menu(const Library *lib) {
    char query[129];
    if (!read_line("Exact keyword (for example sorting): ", query, sizeof(query))) return;
    int *ids = malloc((size_t)lib->count * sizeof(int));
    if (!ids) { puts("Not enough memory."); return; }
    int count = keyword_lookup(lib, query, ids, lib->count);
    if (count < 0) puts("Keyword must fit in 64 bytes.");
    else if (count == 0) puts("No exact keyword tag found. Try text search for partial words.");
    for (int i = 0; i < count; i++) {
        const Book *book = find_book(lib, ids[i], 0);
        printf("ID %d | %s\n", book->id, book->title);
    }
    free(ids);
}

int main(int argc, char **argv) {
    locate_data(argv[0]);
    if (argc > 1 && strcmp(argv[1], "--test") == 0) return run_tests(stdout) ? 0 : 1;
    Library library;
    char error[256];
    library_init(&library);
    if (!library_load(&library, "data/books.csv", error, sizeof(error))) {
        puts(error);
        return 1;
    }
    if (argc > 1) {
        int success = 0;
        if (strcmp(argv[1], "--compare") == 0 || strcmp(argv[1], "--demo") == 0) {
            success = run_complete_comparisons(&library, stdout);
        } else if (strcmp(argv[1], "--export") == 0) {
            success = export_reports(&library, "results");
            puts(success ? "Saved CSV reports in results/." : "Export failed.");
        } else puts("Use --compare, --test or --export.");
        library_free(&library);
        return success ? 0 : 1;
    }
    printf("LibraDAA | Loaded %d books\n", library.count);
    for (;;) {
        int choice;
        puts("\n1 Show books\n2 Search text\n3 Find book by ID\n4 Recommend similar books\n5 Build a reading plan\n6 Exact keyword lookup\n0 Exit");
        if (!read_number("Choice: ", &choice) || choice == 0) break;
        switch (choice) {
            case 1: show_books(&library); break;
            case 2: search_menu(&library); break;
            case 3: find_menu(&library); break;
            case 4: recommend_menu(&library); break;
            case 5: reading_menu(&library); break;
            case 6: keyword_menu(&library); break;
            default: puts("Choose a menu option from 0 to 6.");
        }
    }
    library_free(&library);
    return 0;
}
