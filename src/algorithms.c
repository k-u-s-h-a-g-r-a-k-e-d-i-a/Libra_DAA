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
        high = libraa_fast_pow_mod(base,(long long)m-1,prime,s);
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
int compare_ints(const void *a,const void *b) {
    int x=*(const int*)a,y=*(const int*)b;
    return (x>y)-(x<y);
}
int binary_iter(const void *array,int n,size_t width,const void *key,Compare compare,Counters *s) {
    if(n<0||!width||!compare||!key||(n&&!array)){
        errno=EINVAL;
        return -1;
    }
    const unsigned char *a=array;
    int lo=0,hi=n-1;
    while(lo<=hi) {
        int mid=lo+(hi-lo)/2;
        int order=compare(key,a+(size_t)mid*width);
        step(s);
        if(order==0)return mid;
        if(order>0)lo=mid+1;
        else hi=mid-1;
    }
    return -1;
}
static int find_rec(const unsigned char *a,int lo,int hi,size_t width,const void *key,Compare compare,Counters *s,size_t depth) {
    counters_memory(s,depth*4*sizeof(int));
    if(lo>hi)return -1;
    int mid=lo+(hi-lo)/2;
    int order=compare(key,a+(size_t)mid*width);
    step(s);
    if(order==0)return mid;
    if(order>0)return find_rec(a,mid+1,hi,width,key,compare,s,depth+1);
    return find_rec(a,lo,mid-1,width,key,compare,s,depth+1);
}
int binary_rec(const void *array,int n,size_t width,const void *key,Compare compare,Counters *s) {
    if(n<0||!width||!compare||!key||(n&&!array)){
        errno=EINVAL;
        return -1;
    }
    return find_rec(array,0,n-1,width,key,compare,s,1);
}
static void merge_range(unsigned char *a,unsigned char *tmp,int lo,int hi,size_t width,Compare compare,Counters *s) {
    if(hi-lo<2)return;
    int mid=lo+(hi-lo)/2;
    merge_range(a,tmp,lo,mid,width,compare,s);
    merge_range(a,tmp,mid,hi,width,compare,s);
    int i=lo,j=mid,k=lo;
    while(i<mid&&j<hi) {
        step(s);
        int next=compare(a+(size_t)i*width,a+(size_t)j*width)<=0?i++:j++;
        memcpy(tmp+(size_t)k++*width,a+(size_t)next*width,width);
    }
    while(i<mid)memcpy(tmp+(size_t)k++*width,a+(size_t)i++*width,width);
    while(j<hi)memcpy(tmp+(size_t)k++*width,a+(size_t)j++*width,width);
    memcpy(a+(size_t)lo*width,tmp+(size_t)lo*width,(size_t)(hi-lo)*width);
}
int merge_sort(void *array,int n,size_t width,Compare compare,Counters *s) {
    if(n<0||!width||!compare||(n&&!array)||(size_t)n>(size_t)-1/width){
        errno=EINVAL;
        return 0;
    }
    if(n<2)return 1;
    unsigned char *tmp=malloc((size_t)n*width);
    if(!tmp){
        errno=ENOMEM;
        return 0;
    }
    counters_memory(s,(size_t)n*width);
    merge_range(array,tmp,0,n,width,compare,s);
    free(tmp);
    return 1;
}
static void quick_range(unsigned char *a,int lo,int hi,size_t width,Compare compare,unsigned char *pivot,unsigned char *tmp,Counters *s,size_t depth) {
    counters_memory(s,2*width+depth*6*sizeof(int));
    while(lo<hi) {
        int i=lo,j=hi;
        memcpy(pivot,a+(size_t)(lo+(hi-lo)/2)*width,width);
        while(i<=j) {
            while((step(s),compare(a+(size_t)i*width,pivot)<0))++i;
            while((step(s),compare(a+(size_t)j*width,pivot)>0))--j;
            if(i<=j) {
                if(i<j) {
                    memcpy(tmp,a+(size_t)i*width,width);
                    memcpy(a+(size_t)i*width,a+(size_t)j*width,width);
                    memcpy(a+(size_t)j*width,tmp,width);
                }
                ++i;
                --j;
            }
        }
        if(j-lo<hi-i){
            if(lo<j)quick_range(a,lo,j,width,compare,pivot,tmp,s,depth+1);
            lo=i;
        }
        else {
            if(i<hi)quick_range(a,i,hi,width,compare,pivot,tmp,s,depth+1);
            hi=j;
        }
    }
}
int quick_sort(void *array,int n,size_t width,Compare compare,Counters *s) {
    if(n<0||!width||width>(size_t)-1/2||!compare||(n&&!array)||(size_t)n>(size_t)-1/width){
        errno=EINVAL;
        return 0;
    }
    if(n<2)return 1;
    unsigned char *workspace=malloc(2*width);
    if(!workspace){
        errno=ENOMEM;
        return 0;
    }
    quick_range(array,0,n-1,width,compare,workspace,workspace+width,s,1);
    free(workspace);
    return 1;
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
