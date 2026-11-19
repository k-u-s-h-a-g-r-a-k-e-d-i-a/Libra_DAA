#include "algorithms.h"
#include "workbench.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
const char *matcher_name(Matcher e) {
    static const char *names[] = {
        "Naive", "KMP", "Rabin-Karp", "Automaton"
    };
    return e >= NAIVE && e <= AUTOMATON ? names[e] : "Unknown";
}

static void step(Counters *s) {
    if (s) ++s->comparisons;
}

static void prefix_table(const char *pattern, size_t pattern_length, int *lps, Counters *stats) {
    size_t i = 1;
    int length = 0;
    lps[0] = 0;
    while (i < pattern_length) {
        step(stats);
        if (pattern[i] == pattern[length]) {
            length++;
            lps[i] = length;
            i++;
        }
        else if (length > 0) length = lps[length - 1];
        else lps[i++] = 0;
    }
}

size_t match_text(const char *text, const char *pattern, Matcher engine, size_t *positions, size_t cap, Counters *stats) {
    size_t text_length, pattern_length, count = 0, i, j;
    int *lps;
    if (!text || !pattern || engine < NAIVE || engine > AUTOMATON) {
        errno = EINVAL;
        return 0;
    }
    text_length = strlen(text);
    pattern_length = strlen(pattern);
    if (pattern_length == 0 || pattern_length > text_length) return 0;
    if (pattern_length > 2048) {
        errno = EINVAL;
        return 0;
    }
    if (engine == NAIVE) {
        for (i = 0; i + pattern_length <= text_length; ++i) {
            for (j = 0; j < pattern_length; ++j) {
                step(stats);
                if (text[i+j] != pattern[j]) break;
            }
            if (j == pattern_length) {
                if (positions && count < cap) positions[count] = i;
                ++count;
            }
        }
        return count;
    }
    if (engine == RABIN_KARP) {
        const long long base = 256, prime = 1000000007;
        long long high = 1, pattern_hash = 0, window_hash = 0;
        high = libraa_fast_pow_mod(base, (long long)pattern_length-1, prime, stats);
        for (j = 0; j < pattern_length; ++j) {
            pattern_hash = (pattern_hash * base + (unsigned char)pattern[j]) % prime;
            window_hash = (window_hash * base + (unsigned char)text[j]) % prime;
        }
        for (i = 0; i + pattern_length <= text_length; ++i) {
            step(stats);
            if (pattern_hash == window_hash) {
                for (j = 0; j < pattern_length; ++j) {
                    step(stats);
                    if (text[i+j] != pattern[j]) break;
                }
                if (j == pattern_length) {
                    if (positions && count < cap) positions[count] = i;
                    ++count;
                }
            }
            if (i+pattern_length < text_length) window_hash = ((window_hash - (unsigned char)text[i]*high%prime + prime)*base + (unsigned char)text[i+pattern_length])%prime;
        }
        return count;
    }
    lps = malloc(pattern_length * sizeof(*lps));
    if (!lps) {
        errno = ENOMEM;
        return 0;
    }
    prefix_table(pattern, pattern_length, lps, stats);
    counters_memory(stats, pattern_length * sizeof(*lps));
    if (engine == KMP) {
        j = 0;
        for (i = 0; i < text_length; ++i) {
            while (j > 0) {
                step(stats);
                if (text[i] == pattern[j]) break;
                j = (size_t)lps[j-1];
            }
            step(stats);
            if (text[i] == pattern[j]) ++j;
            if (j == pattern_length) {
                if (positions && count < cap) positions[count] = i+1-pattern_length;
                ++count;
                j = (size_t)lps[j-1];
            }
        }
    }
    else {
        int (*delta)[256] = calloc(pattern_length+1, sizeof(*delta));
        size_t state;
        if (!delta) {
            free(lps);
            errno = ENOMEM;
            return 0;
        }
        counters_memory(stats, pattern_length*sizeof(*lps) + (pattern_length+1)*sizeof(*delta));
        for (state = 0; state <= pattern_length; ++state) {
            int ch;
            for (ch = 0; ch < 256; ++ch) {
                step(stats);
                if (state < pattern_length && ch == (unsigned char)pattern[state]) delta[state][ch] = (int)state+1;
                else delta[state][ch] = state ? delta[lps[state-1]][ch] : 0;
            }
        }
        state = 0;
        for (i = 0; i < text_length; ++i) {
            state = (size_t)delta[state][(unsigned char)text[i]];
            step(stats);
            if (state == pattern_length) {
                if (positions && count < cap) positions[count] = i+1-pattern_length;
                ++count;
            }
        }
        free(delta);
    }
    free(lps);
    return count;
}

int compare_ints(const void *a, const void *b) {
    int x = *(const int*)a, y = *(const int*)b;
    return (x > y)-(x < y);
}

int binary_iter(const void *array, int count, size_t width, const void *key, Compare compare, Counters *stats) {
    if (count < 0 || !width || !compare || !key || (count && !array)) {
        errno = EINVAL;
        return -1;
    }
    const unsigned char *records = array;
    int left = 0, right = count - 1;
    while (left <= right) {
        int middle = left + (right - left) / 2;
        int order = compare(key, records + (size_t)middle * width);
        step(stats);
        if (order == 0) return middle;
        if (order > 0) left = middle + 1;
        else right = middle - 1;
    }
    return -1;
}

static int find_rec(const unsigned char *records, int left, int right, size_t width, const void *key, Compare compare, Counters *stats, size_t depth) {
    counters_memory(stats, depth*4*sizeof(int));
    if (left > right) return -1;
    int middle = left + (right - left) / 2;
    int order = compare(key, records + (size_t)middle * width);
    step(stats);
    if (order == 0) return middle;
    if (order > 0) return find_rec(records, middle + 1, right, width, key, compare, stats, depth+1);
    return find_rec(records, left, middle - 1, width, key, compare, stats, depth+1);
}

int binary_rec(const void *array, int count, size_t width, const void *key, Compare compare, Counters *stats) {
    if (count < 0 || !width || !compare || !key || (count && !array)) {
        errno = EINVAL;
        return -1;
    }
    return find_rec(array, 0, count-1, width, key, compare, stats, 1);
}

static void merge_range(unsigned char *a, unsigned char *tmp, int lo, int hi, size_t width, Compare compare, Counters *s) {
    if (hi-lo < 2) return;
    int mid = lo+(hi-lo)/2;
    merge_range(a, tmp, lo, mid, width, compare, s);
    merge_range(a, tmp, mid, hi, width, compare, s);
    int left = lo;
    int right = mid;
    /* Copy the smaller front record. Taking the left one on a tie keeps the sort stable. */
    for (int out = lo; out < hi; out++) {
        int source;
        if (left == mid) {
            source = right++;
        } else if (right == hi) {
            source = left++;
        } else {
            step(s);
            if (compare(a + left * width, a + right * width) <= 0) {
                source = left++;
            } else {
                source = right++;
            }
        }
        memcpy(tmp + out * width, a + source * width, width);
    }
    memcpy(a+(size_t)lo*width, tmp+(size_t)lo*width, (size_t)(hi-lo)*width);
}

int merge_sort(void *array, int n, size_t width, Compare compare, Counters *s) {
    if (n < 0 || !width || !compare || (n && !array) || (size_t)n > (size_t)-1/width) {
        errno = EINVAL;
        return 0;
    }
    if (n < 2) return 1;
    unsigned char *tmp = malloc((size_t)n*width);
    if (!tmp) {
        errno = ENOMEM;
        return 0;
    }
    counters_memory(s, (size_t)n*width);
    merge_range(array, tmp, 0, n, width, compare, s);
    free(tmp);
    return 1;
}

static void quick_range(unsigned char *a, int lo, int hi, size_t width, Compare compare, unsigned char *pivot, unsigned char *tmp, Counters *s, size_t depth) {
    counters_memory(s, 2*width+depth*6*sizeof(int));
    while (lo < hi) {
        int i = lo, j = hi;
        memcpy(pivot, a+(size_t)(lo+(hi-lo)/2)*width, width);
        while (i <= j) {
            for (;;) {
                step(s);
                if (compare(a + i * width, pivot) >= 0) break;
                i++;
            }
            for (;;) {
                step(s);
                if (compare(a + j * width, pivot) <= 0) break;
                j--;
            }
            if (i <= j) {
                if (i < j) {
                    memcpy(tmp, a+(size_t)i*width, width);
                    memcpy(a+(size_t)i*width, a+(size_t)j*width, width);
                    memcpy(a+(size_t)j*width, tmp, width);
                }
                ++i;
                --j;
            }
        }
        if (j-lo < hi-i) {
            if (lo < j)quick_range(a, lo, j, width, compare, pivot, tmp, s, depth+1);
            lo = i;
        }
        else {
            if (i < hi)quick_range(a, i, hi, width, compare, pivot, tmp, s, depth+1);
            hi = j;
        }
    }
}

int quick_sort(void *array, int n, size_t width, Compare compare, Counters *s) {
    if (n < 0 || !width || width > (size_t)-1/2 || !compare || (n && !array) || (size_t)n > (size_t)-1/width) {
        errno = EINVAL;
        return 0;
    }
    if (n < 2) return 1;
    unsigned char *workspace = malloc(2*width);
    if (!workspace) {
        errno = ENOMEM;
        return 0;
    }
    quick_range(array, 0, n-1, width, compare, workspace, workspace+width, s, 1);
    free(workspace);
    return 1;
}

int lcs(const char *a, const char *b, char *out, size_t cap, Counters *s) {
    size_t n, m, i, j, width;
    int *dp, length, at;
    if (out && cap) out[0] = '\0';
    if (!a || !b) return -1;
    n = strlen(a);
    m = strlen(b);
    width = m+1;
    if (n > 2048 || m > 2048) return -1;
    dp = calloc((n+1)*width, sizeof(*dp));
    if (!dp) return -1;
    counters_memory(s, (n+1)*width*sizeof(*dp));
    for (i = 1; i <= n; ++i) for (j = 1; j <= m; ++j) {
        int top = dp[(i-1)*width+j], left = dp[i*width+j-1];
        step(s);
        if (a[i-1] == b[j-1]) {
            dp[i*width+j] = 1 + dp[(i-1)*width+j-1];
        } else if (top > left) {
            dp[i*width+j] = top;
        } else {
            dp[i*width+j] = left;
        }
    }
    length = dp[n*width+m];
    if (out) {
        if (cap <= (size_t)length) {
            free(dp);
            errno = ENOSPC;
            return -1;
        }
        at = length;
        out[at] = '\0';
        i = n;
        j = m;
        while (i && j) {
            if (a[i-1] == b[j-1]) {
                out[--at] = a[--i];
                --j;
            }
            else if (dp[(i-1)*width+j] >= dp[i*width+j-1]) --i;
            else --j;
        }
    }
    free(dp);
    return length;
}
