#include "library.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif
/* Helpers deal with files;
the algorithms remain ordinary C functions. */
int make_directory(const char *path) {
#ifdef _WIN32
    int status = _mkdir(path);
#else
    int status = mkdir(path, 0755);
#endif
    return status == 0 || errno == EEXIST;
}

void normalize(char *text) {
    size_t read, write = 0;
    int was_space = 1;
    for (read = 0; text[read]; ++read) {
        unsigned char c = (unsigned char)text[read];
        if (c < 128 && isspace(c)) {
            if (!was_space) {
                text[write++] = ' ';
                was_space = 1;
            }
        }
        else if (c >= 32) {
            text[write++] = (char)(c >= 'A' && c <= 'Z'?c+32:c);
            was_space = 0;
        }
    }
    if (write && text[write-1] == ' ') --write;
    text[write] = '\0';
}

void library_init(Library *lib) {
    memset(lib, 0, sizeof(*lib));
}

void library_free(Library *lib) {
    free(lib->books);
    free(lib->index);
    library_init(lib);
}
/* These functions only define ordering. All sorting calls merge_sort(). */
static int compare_books(const void *left, const void *right) {
    const Book *a = left, *b = right;
    return (a->id > b->id)-(a->id < b->id);
}

static int compare_words(const void *left, const void *right) {
    const IndexEntry *a = left, *b = right;
    return strcmp(a->word, b->word);
}

static int compare_entries(const void *left, const void *right) {
    const IndexEntry *a = left, *b = right;
    int order = compare_words(a, b);
    return order?order:(a->book_id > b->book_id)-(a->book_id < b->book_id);
}

static int compare_hits(const void *left, const void *right) {
    const Hit *a = left, *b = right;
    if (a->score != b->score) return a->score > b->score?-1:1;
    return (a->id > b->id)-(a->id < b->id);
}

static int csv_fields(const char *line, char fields[9][2050]) {
    int field = 0, quoted = 0, closed = 0;
    size_t at = 0;
    memset(fields, 0, 9*2050);
    for (size_t i = 0; line[i] && line[i] != '\n' && line[i] != '\r'; ++i) {
        char ch = line[i];
        if (quoted) {
            if (ch == '"') {
                if (line[i+1] == '"') {
                    if (at >= 2049) return 0;
                    fields[field][at++] = '"';
                    ++i;
                }
                else{
                    quoted = 0;
                    closed = 1;
                }
            }
            else {
                if (at >= 2049) return 0;
                fields[field][at++] = ch;
            }
        }
        else if (ch == ',') {
            fields[field][at] = '\0';
            if (++field >= 9) return 0;
            at = 0;
            closed = 0;
        }
        else if (ch == '"') {
            if (at || closed) return 0;
            quoted = 1;
        }
        else {
            if (closed || at >= 2049) return 0;
            fields[field][at++] = ch;
        }
    }
    fields[field][at] = '\0';
    return !quoted && field == 8;
}

static int parse_int(const char *s, int *out) {
    char *end;
    long v;
    errno = 0;
    v = strtol(s, &end, 10);
    if (errno || end == s || *end || v < INT_MIN || v > INT_MAX) return 0;
    *out = (int)v;
    return 1;
}

static int copy_text(char *out, size_t cap, const char *in) {
    if (strlen(in) >= cap) return 0;
    strcpy(out, in);
    return 1;
}

static int build_index(Library *lib) {
    int count = 0;
    for (int i = 0; i < lib->count; ++i)count += lib->books[i].keyword_count;
    IndexEntry *entries = malloc((size_t)count*sizeof(*entries));
    if (!entries) return 0;
    int at = 0;
    for (int i = 0; i < lib->count; ++i)for (int k = 0; k < lib->books[i].keyword_count; ++k) {
        strcpy(entries[at].word, lib->books[i].keywords[k]);
        entries[at++].book_id = lib->books[i].id;
    }
    if (!merge_sort(entries, count, sizeof(*entries), compare_entries, NULL)) {
        free(entries);
        return 0;
    }
    lib->index = entries;
    lib->index_count = count;
    return 1;
}

int library_load(Library *lib, const char *path, char *error, size_t cap) {
    FILE *file = fopen(path,"r");
    Library fresh;
    char line[8194];
    int row = 1, ok = 0, allocated = 0;
    library_init(&fresh);
    if (error && cap)error[0] = '\0';
    if (!file) {
        if (error && cap)snprintf(error, cap,"Cannot open %s; run from the project folder", path);
        return 0;
    }
    if (!fgets(line, sizeof(line), file)) goto fail;
    normalize(line);
    if (strcmp(line,"id,title,author,subject,keywords,description,copies,hours,value") != 0) goto fail;
    while (fgets(line, sizeof(line), file)) {
        char f[9][2050];
        Book b;
        char *part;
        int capacity;
        ++row;
        if (line[0] == '\n' || line[0] == '\r') continue;
        if (strlen(line) > 8192 || !csv_fields(line, f) || fresh.count >= MAX_BOOKS) goto fail;
        memset(&b, 0, sizeof(b));
        if (!parse_int(f[0], &b.id) || !parse_int(f[6], &b.copies) || !parse_int(f[7], &b.hours) || !parse_int(f[8], &b.value)) goto fail;
        for (int i = 1; i <= 5; ++i)normalize(f[i]);
        if (b.id < 1 || b.copies < 0 || b.copies > 100 || b.hours < 1 || b.hours > 10000 || b.value < 0 || b.value > 1000000 || !f[1][0] || !f[2][0] || !f[3][0]) goto fail;
        if (!copy_text(b.title, sizeof(b.title), f[1]) || !copy_text(b.author, sizeof(b.author), f[2]) || !copy_text(b.subject, sizeof(b.subject), f[3]) || !copy_text(b.description, sizeof(b.description), f[5])) goto fail;
        part = strtok(f[4],";");
        while (part) {
            normalize(part);
            if (*part) {
                int duplicate = 0;
                for (int k = 0; k < b.keyword_count; ++k)if (strcmp(b.keywords[k], part) == 0) duplicate = 1;
                if (!duplicate) {
                    if (b.keyword_count == MAX_KEYWORDS || !copy_text(b.keywords[b.keyword_count], 65, part)) goto fail;
                    ++b.keyword_count;
                }
            }
            part = strtok(NULL,";");
        }
        if (!b.keyword_count) goto fail;
        if (fresh.count == allocated) {
            capacity = allocated?allocated*2:16;
            if (capacity > MAX_BOOKS) capacity = MAX_BOOKS;
            Book *next = realloc(fresh.books, (size_t)capacity*sizeof(Book));
            if (!next) goto fail;
            fresh.books = next;
            allocated = capacity;
        }
        fresh.books[fresh.count++] = b;
    }
    if (ferror(file) || !fresh.count) goto fail;
    if (!merge_sort(fresh.books, fresh.count, sizeof(Book), compare_books, NULL)) goto fail;
    for (int i = 1; i < fresh.count; ++i)if (fresh.books[i-1].id == fresh.books[i].id) goto fail;
    if (!build_index(&fresh)) goto fail;
    fresh.reader_count = 1;
    fresh.readers[0].id = 101;
    strcpy(fresh.readers[0].name,"Kushagra");
    fresh.readers[0].history[0] = fresh.books[0].id;
    fresh.readers[0].history_count = 1;
    if (fresh.count > 1) {
        fresh.reader_count = 2;
        fresh.readers[1].id = 102;
        strcpy(fresh.readers[1].name,"Reader B");
        fresh.readers[1].history[0] = fresh.books[1].id;
        fresh.readers[1].history_count = 1;
        fresh.citation_count = 1;
        fresh.citations[0] = (Citation) {
            fresh.books[1].id, fresh.books[0].id
        };
    }
    library_free(lib);
    *lib = fresh;
    ok = 1;
    fail:     fclose(file);
    if (!ok) {
        library_free(&fresh);
        if (error && cap)snprintf(error, cap,"Invalid CSV or insufficient memory at/after row %d (check header, limits and duplicate IDs)", row);
    }
    return ok;
}

const Book *find_book(const Library *lib, int id, int recursive) {
    Book key = {0};
    key.id = id;
    int position;
    if (recursive) {
        position = binary_rec(lib->books, lib->count, sizeof(Book), &key, compare_books, NULL);
    } else {
        position = binary_iter(lib->books, lib->count, sizeof(Book), &key, compare_books, NULL);
    }
    if (position < 0) return NULL;
    return &lib->books[position];
}

int keyword_lookup(const Library *lib, const char *raw, int *ids, int cap) {
    IndexEntry key = {0};
    int count = 0;
    if (strlen(raw) >= sizeof(key.word) || cap < 0) return -1;
    strcpy(key.word, raw);
    normalize(key.word);
    int at = binary_iter(lib->index, lib->index_count, sizeof(IndexEntry), &key, compare_words, NULL);
    if (at < 0) return 0;
    while (at > 0 && strcmp(lib->index[at-1].word, key.word) == 0)--at;
    while (at < lib->index_count && strcmp(lib->index[at].word, key.word) == 0) {
        if (count == cap) return -1;
        ids[count++] = lib->index[at++].book_id;
    }
    return count;
}

int search_books(const Library *lib, const char *raw, Matcher engine,
                 Hit *hits, int capacity, Counters *stats) {
    char query[129];
    int count = 0;
    errno = 0;
    if (engine < NAIVE || engine > AUTOMATON || strlen(raw) > 128 || capacity < lib->count) {
        errno = EINVAL;
        return -1;
    }
    strcpy(query, raw);
    normalize(query);
    if (!query[0]) return -1;

    for (int i = 0; i < lib->count; i++) {
        const Book *book = &lib->books[i];
        int title_matches = match_text(book->title, query, engine, NULL, 0, stats);
        int description_matches = match_text(book->description, query, engine, NULL, 0, stats);
        int keyword_matches = 0;
        for (int k = 0; k < book->keyword_count; k++) {
            keyword_matches += match_text(book->keywords[k], query, engine, NULL, 0, stats);
        }
        if (errno == ENOMEM) return -1;

        /* LIVE EDIT: change a weight, then search for the same word again. */
        double score = 3.0 * title_matches + 2.0 * keyword_matches + description_matches;
        if (score > 0) {
            hits[count].id = book->id;
            hits[count].score = score;
            count++;
        }
    }
    if (!merge_sort(hits, count, sizeof(Hit), compare_hits, stats)) return -1;
    return count;
}

int recommend_books(const Library *lib, int id, Hit *hits, int capacity) {
    const Book *target = find_book(lib, id, 0);
    int count = 0;
    if (!target || capacity < lib->count) return -1;

    for (int i = 0; i < lib->count; i++) {
        const Book *book = &lib->books[i];
        if (book->id == id) continue;

        int shared = 0;
        for (int j = 0; j < target->keyword_count; j++) {
            for (int k = 0; k < book->keyword_count; k++) {
                if (strcmp(target->keywords[j], book->keywords[k]) == 0) shared++;
            }
        }
        int same_subject = strcmp(target->subject, book->subject) == 0;
        if (shared == 0 && !same_subject) continue;

        int total_keywords = target->keyword_count + book->keyword_count - shared;
        double jaccard = (double)shared / total_keywords;
        int title_lcs = lcs(target->title, book->title, NULL, 0, NULL);
        if (title_lcs < 0) return -1;
        size_t longest = strlen(target->title);
        size_t book_length = strlen(book->title);
        if (book_length > longest) longest = book_length;
        double title_similarity = (double)title_lcs / longest;

        /* LIVE EDIT: keyword overlap, same subject, and title similarity. */
        hits[count].id = book->id;
        hits[count].score = .5 * jaccard + .3 * same_subject + .2 * title_similarity;
        count++;
    }
    if (!merge_sort(hits, count, sizeof(Hit), compare_hits, NULL)) return -1;
    if (count > 5) return 5;
    return count;
}

static void csv_quote(FILE *f, const char *s) {
    fputc('"', f);
    for (; *s; ++s) {
        if (*s == '"')fputc('"', f);
        fputc(*s, f);
    }
    fputc('"', f);
}

int export_reports(const Library *lib, const char *folder) {
    char path[512];
    FILE *cat = NULL, *idx = NULL, *rec = NULL, *rel = NULL;
    Hit *hits = NULL;
    int ok = 0;
    if (strlen(folder) > 400 || !make_directory(folder)) return 0;
    snprintf(path, sizeof(path),"%s/catalog.csv", folder);
    cat = fopen(path,"w");
    snprintf(path, sizeof(path),"%s/index.csv", folder);
    idx = fopen(path,"w");
    snprintf(path, sizeof(path),"%s/recommendations.csv", folder);
    rec = fopen(path,"w");
    snprintf(path, sizeof(path),"%s/relationships.csv", folder);
    rel = fopen(path,"w");
    if (!cat || !idx || !rec || !rel) goto done;
    fprintf(cat,"id,title,author,subject,keywords,description,copies,hours,value\n");
    for (int i = 0; i < lib->count; ++i) {
        const Book *b = &lib->books[i];
        char keys[2112] = "";
        for (int k = 0; k < b->keyword_count; ++k) {
            if (k)strcat(keys,";");
            strcat(keys, b->keywords[k]);
        }
        fprintf(cat,"%d,", b->id);
        csv_quote(cat, b->title);
        fputc(',', cat);
        csv_quote(cat, b->author);
        fputc(',', cat);
        csv_quote(cat, b->subject);
        fputc(',', cat);
        csv_quote(cat, keys);
        fputc(',', cat);
        csv_quote(cat, b->description);
        fprintf(cat,",%d,%d,%d\n", b->copies, b->hours, b->value);
    }
    fprintf(idx,"keyword,book_id\n");
    for (int i = 0; i < lib->index_count; ++i) {
        csv_quote(idx, lib->index[i].word);
        fprintf(idx,",%d\n", lib->index[i].book_id);
    }
    fprintf(rec,"source_id,recommended_id,score\n");
    hits = malloc((size_t)lib->count*sizeof(*hits));
    if (!hits) goto done;
    int n = recommend_books(lib, lib->books[0].id, hits, lib->count);
    if (n < 0) goto done;
    for (int i = 0; i < n; ++i) fprintf(rec,"%d,%d,%.12g\n", lib->books[0].id, hits[i].id, hits[i].score);
    fprintf(rel,"type,from,to\n");
    for (int i = 0; i < lib->reader_count; ++i)for (int j = 0; j < lib->readers[i].history_count; ++j) fprintf(rel,"read,%d,%d\n", lib->readers[i].id, lib->readers[i].history[j]);
    for (int i = 0; i < lib->citation_count; ++i) fprintf(rel,"cites,%d,%d\n", lib->citations[i].from, lib->citations[i].to);
    ok = !ferror(cat) && !ferror(idx) && !ferror(rec) && !ferror(rel);
    done:     free(hits);
    if (cat && fclose(cat)) ok = 0;
    if (idx && fclose(idx)) ok = 0;
    if (rec && fclose(rec)) ok = 0;
    if (rel && fclose(rel)) ok = 0;
    return ok;
}
