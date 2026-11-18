#ifndef LIBRARY_H
#define LIBRARY_H
#include "algorithms.h"
#include "workbench.h"
#include <stdio.h>
#define MAX_BOOKS 10000
#define MAX_KEYWORDS 32
/* One row from data/books.csv. */
typedef struct {
    int id, copies, hours, value;
    char title[257], author[129], subject[129], description[2049];
    char keywords[MAX_KEYWORDS][65];
    int keyword_count;
} Book;
typedef struct {
    char word[65];
    int book_id;
} IndexEntry;
typedef struct {
    int id;
    double score;
} Hit;
typedef struct {
    int id;
    char name[64];
    int history[8], history_count;
} Reader;
typedef struct {
    int from, to;
} Citation;
typedef struct {
    Book *books;
    int *ids; /* sorted ID array; position matches books[] */
    int count;
    IndexEntry *index;
    int index_count;
    Reader readers[2];
    int reader_count;
    Citation citations[1];
    int citation_count;
} Library;
void library_init(Library *lib);
void library_free(Library *lib);
int library_load(Library *lib, const char *path, char *error, size_t error_capacity);
const Book *find_book(const Library *lib, int id, int recursive);
int keyword_lookup(const Library *lib, const char *word, int *ids, int capacity);
int search_books(const Library *lib, const char *query, Matcher engine, Hit *hits, int capacity, Counters *stats);
int recommend_books(const Library *lib, int id, Hit *hits, int capacity);
int export_reports(const Library *lib, const char *folder);
void normalize(char *text);
int make_directory(const char *path);
int run_lab(FILE *out);
int run_scenarios(const Library *lib, FILE *out);
int run_benchmarks(FILE *out);
int run_tests(FILE *out);
#endif
