#include "algorithms.h"
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
static void prefix_table(const char *p, size_t m, int *lps, Counters *s) {
    size_t i = 1;
    int length = 0;
    lps[0] = 0;
    while (i < m) {
        step(s);
        if (p[i] == p[length]) lps[i++] = ++length;
        else if (length > 0) length = lps[length - 1];
        else lps[i++] = 0;
    }
}
size_t match_text(const char *t, const char *p, Matcher e,                   size_t *positions, size_t cap, Counters *s) {
    size_t n, m, count = 0, i, j;
    int *lps;
    if (!t || !p || e < NAIVE || e > AUTOMATON) {
        errno = EINVAL;
        return 0;
    }
    n = strlen(t);
    m = strlen(p);
    if (m == 0 || m > n) return 0;
    if (m > 2048) {
        errno = EINVAL;
        return 0;
    }
    if (e == NAIVE) {
        for (i = 0; i + m <= n; ++i) {
            for (j = 0; j < m; ++j) {
                step(s);
                if (t[i+j] != p[j]) break;
            }
            if (j == m) {
                if (positions && count < cap) positions[count] = i;
                ++count;
            }
        }
        return count;
    }
    if (e == RABIN_KARP) {
        const long long base = 256, prime = 1000000007;
        long long high = 1, ph = 0, th = 0;
        for (j = 1; j < m; ++j) high = high * base % prime;
        for (j = 0; j < m; ++j) {
            ph = (ph * base + (unsigned char)p[j]) % prime;
            th = (th * base + (unsigned char)t[j]) % prime;
        }
        for (i = 0; i + m <= n; ++i) {
            step(s);
            if (ph == th) {
                for (j = 0; j < m; ++j) {
                    step(s);
                    if (t[i+j] != p[j]) break;
                }
                if (j == m) {
                    if (positions && count < cap) positions[count] = i;
                    ++count;
                }
            }
            if (i+m < n) th = ((th - (unsigned char)t[i]*high%prime + prime)*base + (unsigned char)t[i+m])%prime;
        }
        return count;
    }
    lps = malloc(m * sizeof(*lps));
    if (!lps) {
        errno = ENOMEM;
        return 0;
    }
    prefix_table(p, m, lps, s);
    counters_memory(s, m * sizeof(*lps));
    if (e == KMP) {
        j = 0;
        for (i = 0; i < n; ++i) {
            while (j > 0) {
                step(s);
                if (t[i] == p[j]) break;
                j = (size_t)lps[j-1];
            }
            step(s);
            if (t[i] == p[j]) ++j;
            if (j == m) {
                if (positions && count < cap) positions[count] = i+1-m;
                ++count;
                j = (size_t)lps[j-1];
            }
        }
    }
    else {
        int (*delta)[256] = calloc(m+1, sizeof(*delta));
        size_t state;
        if (!delta) {
            free(lps);
            errno = ENOMEM;
            return 0;
        }
        counters_memory(s, m*sizeof(*lps) + (m+1)*sizeof(*delta));
        for (state = 0; state <= m; ++state) {
            int ch;
            for (ch = 0; ch < 256; ++ch) {
                step(s);
                if (state < m && ch == (unsigned char)p[state]) delta[state][ch] = (int)state+1;
                else delta[state][ch] = state ? delta[lps[state-1]][ch] : 0;
            }
        }
        state = 0;
        for (i = 0; i < n; ++i) {
            state = (size_t)delta[state][(unsigned char)t[i]];
            step(s);
            if (state == m) {
                if (positions && count < cap) positions[count] = i+1-m;
                ++count;
            }
        }
        free(delta);
    }
    free(lps);
    return count;
}
int binary_iter(const int *a, int n, int key, Counters *s) {
    int lo = 0, hi = n-1;
    while (lo <= hi) {
        int mid = lo + (hi-lo)/2;
        step(s);
        if (a[mid] == key) return mid;
        if (a[mid] < key) lo = mid+1;
        else hi = mid-1;
    }
    return -1;
}
static int find_rec(const int *a, int lo, int hi, int key, Counters *s, size_t depth) {
    int mid;
    counters_memory(s, depth * 4 * sizeof(int));
    if (lo > hi) return -1;
    mid = lo + (hi-lo)/2;
    step(s);
    if (a[mid] == key) return mid;
    if (a[mid] < key) return find_rec(a, mid+1, hi, key, s, depth+1);
    return find_rec(a, lo, mid-1, key, s, depth+1);
}
int binary_rec(const int *a, int n, int key, Counters *s) {
    return find_rec(a, 0, n-1, key, s, 1);
}
static void merge_range(int *a, int *tmp, int lo, int hi, Counters *s) {
    int mid, i, j, k;
    if (hi-lo < 2) return;
    mid = lo + (hi-lo)/2;
    merge_range(a, tmp, lo, mid, s);
    merge_range(a, tmp, mid, hi, s);
    i=lo;
    j=mid;
    k=lo;
    while (i<mid && j<hi) {
        step(s);
        tmp[k++] = a[i]<=a[j] ? a[i++] : a[j++];
    }
    while (i<mid) tmp[k++]=a[i++];
    while (j<hi) tmp[k++]=a[j++];
    for (k=lo; k<hi; ++k) a[k]=tmp[k];
}
int merge_ints(int *a, int n, Counters *s) {
    int *tmp;
    if (n<0 || (n && !a)) return 0;
    if (n<2) return 1;
    tmp=malloc((size_t)n*sizeof(*tmp));
    if (!tmp) return 0;
    counters_memory(s, (size_t)n*sizeof(*tmp));
    merge_range(a,tmp,0,n,s);
    free(tmp);
    return 1;
}
static void quick_range(int *a, int lo, int hi, Counters *s, size_t depth) {
    counters_memory(s, depth*6*sizeof(int));
    while (lo<hi) {
        int i=lo, j=hi, pivot=a[lo+(hi-lo)/2];
        while (i<=j) {
            while ((step(s), a[i]<pivot)) ++i;
            while ((step(s), a[j]>pivot)) --j;
            if (i<=j) {
                int t=a[i];
                a[i]=a[j];
                a[j]=t;
                ++i;
                --j;
            }
        }
        if (j-lo<hi-i) {
            if (lo<j) quick_range(a,lo,j,s,depth+1);
            lo=i;
        }
        else {
            if (i<hi) quick_range(a,i,hi,s,depth+1);
            hi=j;
        }
    }
}
void quick_ints(int *a, int n, Counters *s) {
    if (a && n>1) quick_range(a,0,n-1,s,1);
}
int lcs(const char *a, const char *b, char *out, size_t cap, Counters *s) {
    size_t n, m, i, j, width;
    int *dp, length, at;
    if (out && cap) out[0]='\0';
    if (!a || !b) return -1;
    n=strlen(a);
    m=strlen(b);
    width=m+1;
    if (n>2048 || m>2048) return -1;
    dp=calloc((n+1)*width,sizeof(*dp));
    if (!dp) return -1;
    counters_memory(s,(n+1)*width*sizeof(*dp));
    for (i=1; i<=n; ++i) for (j=1; j<=m; ++j) {
        int top=dp[(i-1)*width+j], left=dp[i*width+j-1];
        step(s);
        dp[i*width+j]=a[i-1]==b[j-1] ? 1+dp[(i-1)*width+j-1] : (top>left?top:left);
    }
    length=dp[n*width+m];
    if (out) {
        if (cap <= (size_t)length) {
            free(dp);
            errno=ENOSPC;
            return -1;
        }
        at=length;
        out[at]='\0';
        i=n;
        j=m;
        while (i && j) {
            if (a[i-1]==b[j-1]) {
                out[--at]=a[--i];
                --j;
            }
            else if (dp[(i-1)*width+j]>=dp[i*width+j-1]) --i;
            else --j;
        }
    }
    free(dp);
    return length;
}
