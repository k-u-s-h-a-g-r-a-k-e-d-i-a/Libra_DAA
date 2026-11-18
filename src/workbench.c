// src/workbench/workbench.c — Full DAA syllabus implementations in pure C.
// PBL handbook lines 6-39 (Common Syllabus Implementation Standard).
#include "workbench.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <limits.h>
// ============================================================================
// 1. DIVIDE AND CONQUER
// ============================================================================
static int max_crossing_sum(const int* arr, int low, int mid, int high,                             int* out_start, int* out_end, Counters* c) {
    int left_sum = INT_MIN;
    int sum = 0;
    int max_left = mid;
    for (int i = mid; i >= low; --i) {
        sum += arr[i];
        if (c) c->comparisons++;
        if (sum > left_sum) {
            left_sum = sum;
            max_left = i;
        }
    }
    int right_sum = INT_MIN;
    sum = 0;
    int max_right = mid + 1;
    for (int j = mid + 1; j <= high; ++j) {
        sum += arr[j];
        if (c) c->comparisons++;
        if (sum > right_sum) {
            right_sum = sum;
            max_right = j;
        }
    }
    if (out_start) *out_start = max_left;
    if (out_end) *out_end = max_right;
    return left_sum + right_sum;
}
int libraa_max_subarray_dc(const int* arr, int low, int high, int* out_start, int* out_end, Counters* c) {
    if(!arr||low<0||high<low){
        errno=EINVAL;
        return 0;
    }
    if (low == high) {
        if (out_start) *out_start = low;
        if (out_end) *out_end = high;
        return arr[low];
    }
    int mid = low + (high - low) / 2;
    int left_s, left_e;
    int left_sum = libraa_max_subarray_dc(arr, low, mid, &left_s, &left_e, c);
    int right_s, right_e;
    int right_sum = libraa_max_subarray_dc(arr, mid + 1, high, &right_s, &right_e, c);
    int cross_s, cross_e;
    int cross_sum = max_crossing_sum(arr, low, mid, high, &cross_s, &cross_e, c);
    if (c) c->comparisons += 2;
    if (left_sum >= right_sum && left_sum >= cross_sum) {
        if (out_start) *out_start = left_s;
        if (out_end) *out_end = left_e;
        return left_sum;
    }
    else if (right_sum >= left_sum && right_sum >= cross_sum) {
        if (out_start) *out_start = right_s;
        if (out_end) *out_end = right_e;
        return right_sum;
    }
    else {
        if (out_start) *out_start = cross_s;
        if (out_end) *out_end = cross_e;
        return cross_sum;
    }
}
int libraa_max_subarray_kadane(const int* arr, int n, int* out_start, int* out_end, Counters* c) {
    if (!arr || n <= 0) return 0;
    int max_so_far = arr[0];
    int curr_max = arr[0];
    int start = 0, end = 0, s = 0;
    for (int i = 1; i < n; ++i) {
        if (c) c->comparisons++;
        if (arr[i] > curr_max + arr[i]) {
            curr_max = arr[i];
            s = i;
        }
        else {
            curr_max += arr[i];
        }
        if (c) c->comparisons++;
        if (curr_max > max_so_far) {
            max_so_far = curr_max;
            start = s;
            end = i;
        }
    }
    if (out_start) *out_start = start;
    if (out_end) *out_end = end;
    return max_so_far;
}
long long libraa_fast_pow_mod(long long base,long long exp,long long mod,Counters*c) {
    long long result;
    if(exp<0 || mod<1 || mod>2000000000LL) {
        errno=EINVAL;
        return -1;
    }
    result=1%mod;
    base=(base%mod+mod)%mod;
    while(exp) {
        if(exp&1) result=result*base%mod;
        base=base*base%mod;
        exp/=2;
        if(c)c->hash_ops+=2;
    }
    return result;
}
int libraa_celebrity_find(const int* matrix, int n, Counters* c) {
    if (!matrix || n <= 0) return -1;
    // Elimination phase: O(n)
    int candidate = 0;
    for (int i = 1; i < n; ++i) {
        if (c) c->comparisons++;
        // matrix[candidate][i]: candidate knows i
        if (matrix[candidate * n + i]) {
            candidate = i;
        }
    }
    // Verification phase: candidate knows nobody, and everyone knows candidate
    for (int i = 0; i < n; ++i) {
        if (i != candidate) {
            if (c) c->comparisons += 2;
            if (matrix[candidate * n + i] || !matrix[i * n + candidate]) {
                return -1;
                // No celebrity
            }
        }
    }
    return candidate;
}
void libraa_matrix_multiply_classical(int n, const double* A, const double* B, double* C, Counters* c) {
    if(n<1||n>128||!A||!B||!C){
        errno=EINVAL;
        return;
    }
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            double sum = 0.0;
            for (int k = 0; k < n; ++k) {
                sum += A[i * n + k] * B[k * n + j];
                if (c) {
                    c->hash_ops++;
                    // multiplications
                    c->comparisons++;
                    // additions
                }
            }
            C[i * n + j] = sum;
        }
    }
}
static void mat_op(int n,const double*a,const double*b,double*out,int sign,Counters*c) {
    for(int i=0;i<n*n;++i){
        out[i]=a[i]+sign*b[i];
        if(c)c->comparisons++;
    }
}
static int strassen_rec(int n,const double*a,const double*b,double*out,Counters*c) {
    if(n==1){
        out[0]=a[0]*b[0];
        if(c)c->hash_ops++;
        return 1;
    }
    int h=n/2,z=h*h;
    double *space=calloc((size_t)17*z,sizeof(double));
    if(!space){
        errno=ENOMEM;
        return 0;
    }
    double *A[4],*B[4],*M[7],*x=space+15*z,*y=space+16*z;
    for(int q=0;q<4;++q){
        A[q]=space+q*z;
        B[q]=space+(q+4)*z;
        for(int i=0;i<h;++i)for(int j=0;j<h;++j){
            A[q][i*h+j]=a[(i+q/2*h)*n+j+q%2*h];
            B[q][i*h+j]=b[(i+q/2*h)*n+j+q%2*h];
        }
    }
    for(int q=0;q<7;++q)M[q]=space+(8+q)*z;
    int ok=1;
    mat_op(h,A[0],A[3],x,1,c);
    mat_op(h,B[0],B[3],y,1,c);
    ok=ok&&strassen_rec(h,x,y,M[0],c);
    mat_op(h,A[2],A[3],x,1,c);
    ok=ok&&strassen_rec(h,x,B[0],M[1],c);
    mat_op(h,B[1],B[3],y,-1,c);
    ok=ok&&strassen_rec(h,A[0],y,M[2],c);
    mat_op(h,B[2],B[0],y,-1,c);
    ok=ok&&strassen_rec(h,A[3],y,M[3],c);
    mat_op(h,A[0],A[1],x,1,c);
    ok=ok&&strassen_rec(h,x,B[3],M[4],c);
    mat_op(h,A[2],A[0],x,-1,c);
    mat_op(h,B[0],B[1],y,1,c);
    ok=ok&&strassen_rec(h,x,y,M[5],c);
    mat_op(h,A[1],A[3],x,-1,c);
    mat_op(h,B[2],B[3],y,1,c);
    ok=ok&&strassen_rec(h,x,y,M[6],c);
    if(ok)for(int i=0;i<h;++i)for(int j=0;j<h;++j){
        int k=i*h+j;
        out[i*n+j]=M[0][k]+M[3][k]-M[4][k]+M[6][k];
        out[i*n+j+h]=M[2][k]+M[4][k];
        out[(i+h)*n+j]=M[1][k]+M[3][k];
        out[(i+h)*n+j+h]=M[0][k]-M[1][k]+M[2][k]+M[5][k];
    }
    free(space);
    return ok;
}
void libraa_matrix_multiply_strassen(int n,const double*A,const double*B,double*C,Counters*c){
    if(n<1||n>128||!A||!B||!C){
        errno=EINVAL;
        return;
    }
    int p=1;
    while(p<n)p*=2;
    double *space=calloc((size_t)3*p*p,sizeof(double));
    if(!space){
        errno=ENOMEM;
        return;
    }
    double *x=space,*y=space+p*p,*out=space+2*p*p;
    for(int i=0;i<n;++i)for(int j=0;j<n;++j){
        x[i*p+j]=A[i*n+j];
        y[i*p+j]=B[i*n+j];
    }
    if(strassen_rec(p,x,y,out,c))for(int i=0;i<n;++i)for(int j=0;j<n;++j)C[i*n+j]=out[i*p+j];
    free(space);
}
// ============================================================================
// 2. GREEDY ALGORITHMS
// ============================================================================
int libraa_activity_selection(Activity* activities, int n, int* out_selected, Counters* c) {
    if(!out_selected||n<0||n>256||(n&&!activities)){
        errno=EINVAL;
        return -1;
    }
    if (!activities || n <= 0) return 0;
    // Sort by finish time ascending (bubble sort or quick sort)
    for (int i = 0; i < n - 1; ++i) {
        for (int j = 0; j < n - i - 1; ++j) {
            if (c) c->comparisons++;
            if (activities[j].finish > activities[j + 1].finish) {
                Activity tmp = activities[j];
                activities[j] = activities[j + 1];
                activities[j + 1] = tmp;
            }
        }
    }
    for(int i=0;i<n;++i)if(activities[i].start<0||activities[i].finish<=activities[i].start){
        errno=EINVAL;
        return -1;
    }
    int count = 0;
    out_selected[count++] = activities[0].id;
    int last_finish = activities[0].finish;
    for (int i = 1; i < n; ++i) {
        if (c) c->comparisons++;
        if (activities[i].start >= last_finish) {
            out_selected[count++] = activities[i].id;
            last_finish = activities[i].finish;
        }
    }
    return count;
}
int libraa_interval_partitioning(const Activity* intervals, int n, int* out_room_assigned, Counters* c) {
    if(n<0||n>256||(n&&!intervals)){
        errno=EINVAL;
        return -1;
    }
    if (!intervals || n <= 0) return 0;
    for(int i=0;i<n;++i)if(intervals[i].start<0||intervals[i].finish<=intervals[i].start){
        errno=EINVAL;
        return -1;
    }
    Activity sorted[256];
    int count = n < 256 ? n : 256;
    for (int i = 0; i < count; ++i) sorted[i] = intervals[i];
    // Sort intervals by start time ascending
    for (int i = 0; i < count - 1; ++i) {
        for (int j = 0; j < count - i - 1; ++j) {
            if (c) c->comparisons++;
            if (sorted[j].start > sorted[j + 1].start) {
                Activity tmp = sorted[j];
                sorted[j] = sorted[j + 1];
                sorted[j + 1] = tmp;
            }
        }
    }
    // room_finish[r] stores the finish time of the last activity in room r
    int room_finish[256];
    int num_rooms = 0;
    for (int i = 0; i < count; ++i) {
        int assigned_room = -1;
        for (int r = 0; r < num_rooms; ++r) {
            if (c) c->comparisons++;
            if (room_finish[r] <= sorted[i].start) {
                assigned_room = r;
                break;
            }
        }
        if (assigned_room == -1) {
            assigned_room = num_rooms++;
        }
        room_finish[assigned_room] = sorted[i].finish;
        if (out_room_assigned) {
            for (int k = 0; k < n; ++k) {
                if (intervals[k].id == sorted[i].id) {
                    out_room_assigned[k] = assigned_room + 1;
                    // 1-indexed room
                    break;
                }
            }
        }
    }
    return num_rooms;
}
double libraa_fractional_knapsack(KnapsackItem* items, int n, double capacity, double* out_take_fractions, Counters* c) {
    if(n<0||n>100||capacity<0||(n&&!items)){
        errno=EINVAL;
        return NAN;
    }
    for(int i=0;i<n;++i)if(items[i].weight<=0||items[i].value<0||items[i].id<0||items[i].id>=n){
        errno=EINVAL;
        return NAN;
    }
    if(out_take_fractions)memset(out_take_fractions,0,(size_t)n*sizeof(double));
    if (!items || n <= 0 || capacity <= 0.0) return 0.0;
    // Sort by value/weight ratio descending
    for (int i = 0; i < n - 1; ++i) {
        for (int j = 0; j < n - i - 1; ++j) {
            double r1 = items[j].value / items[j].weight;
            double r2 = items[j + 1].value / items[j + 1].weight;
            if (c) c->comparisons++;
            if (r1 < r2) {
                KnapsackItem tmp = items[j];
                items[j] = items[j + 1];
                items[j + 1] = tmp;
            }
        }
    }
    double current_weight = 0.0;
    double total_value = 0.0;
    for (int i = 0; i < n; ++i) {
        if (c) c->comparisons++;
        if (current_weight + items[i].weight <= capacity) {
            current_weight += items[i].weight;
            total_value += items[i].value;
            if (out_take_fractions) out_take_fractions[items[i].id] = 1.0;
        }
        else {
            double remain = capacity - current_weight;
            double frac = remain / items[i].weight;
            total_value += items[i].value * frac;
            if (out_take_fractions) out_take_fractions[items[i].id] = frac;
            break;
        }
    }
    return total_value;
}
static void make_codes(const HuffmanTree*t,int node,char*path,int depth,char codes[256][256]){
    if(t->left[node]<0){
        if(depth==0)path[depth++]='0';
        path[depth]='\0';
        strcpy(codes[t->symbol[node]],path);
        return;
    }
    path[depth]='0';
    make_codes(t,t->left[node],path,depth+1,codes);
    path[depth]='1';
    make_codes(t,t->right[node],path,depth+1,codes);
}
int libraa_huffman_encode(const char*text,char*out,size_t cap,HuffmanTree*t,Counters*c){
    int freq[511]={
        0
    },active[511],count=0,nodes=0;
    char codes[256][256]={
        {
            0
        }
    },path[256];
    size_t bits=0;
    if(!text||!t||!out||cap==0){
        errno=EINVAL;
        return -1;
    }
    out[0]='\0';
    t->root=-1;
    if(strlen(text)>2048){
        errno=EINVAL;
        return -1;
    }
    int symbol_freq[256]={
        0
    };
    for(const unsigned char*p=(const unsigned char*)text;*p;++p)++symbol_freq[*p];
    count=nodes=0;
    for(int ch=0;ch<256;++ch)if(symbol_freq[ch]){
        t->symbol[nodes]=ch;
        t->left[nodes]=t->right[nodes]=-1;
        freq[nodes]=symbol_freq[ch];
        active[count++]=nodes++;
    }
    if(!count)return 0;
    while(count>1){
        int a=0,b=1;
        if(freq[active[a]]>freq[active[b]]){
            int tmp=a;
            a=b;
            b=tmp;
        }
        for(int i=2;i<count;++i){
            if(c)c->comparisons++;
            if(freq[active[i]]<freq[active[a]]){
                b=a;
                a=i;
            }
            else if(freq[active[i]]<freq[active[b]])b=i;
        }
        int node=nodes++;
        t->symbol[node]=-1;
        t->left[node]=active[a];
        t->right[node]=active[b];
        freq[node]=freq[active[a]]+freq[active[b]];
        active[a]=node;
        active[b]=active[--count];
    }
    t->root=active[0];
    make_codes(t,t->root,path,0,codes);
    for(const unsigned char*p=(const unsigned char*)text;*p;++p){
        size_t n=strlen(codes[*p]);
        if(bits+n>=cap){
            errno=ENOSPC;
            out[0]='\0';
            return -1;
        }
        memcpy(out+bits,codes[*p],n);
        bits+=n;
    }
    out[bits]='\0';
    return (int)bits;
}
int libraa_huffman_decode(const HuffmanTree*t,const char*bits,char*out,size_t cap){
    if(!t||!bits||!out||!cap){
        errno=EINVAL;
        return -1;
    }
    size_t count=0;
    int at=t->root;
    out[0]='\0';
    if(at<0)return bits[0]?-1:0;
    for(size_t i=0;bits[i];++i){
        if(bits[i]!='0'&&bits[i]!='1')return -1;
        if(t->left[t->root]>=0)at=bits[i]=='0'?t->left[at]:t->right[at];
        else if(bits[i]!='0')return -1;
        if(at<0||at>510)return -1;
        if(t->left[at]<0){
            if(count+1>=cap)return -1;
            out[count++]=(char)t->symbol[at];
            at=t->root;
        }
    }
    if(at!=t->root)return -1;
    out[count]='\0';
    return (int)count;
}
void libraa_dijkstra(int n, int src, const double* adj, double* out_dist, int* out_parent, Counters* c) {
    if(n<1||n>128||src<0||src>=n||!adj||!out_dist){
        errno=EINVAL;
        return;
    }
    for(int i=0;i<n*n;++i)if(adj[i]<0){
        errno=EINVAL;
        return;
    }
    bool visited[128] = {
        false
    };
    for (int i = 0; i < n; ++i) {
        out_dist[i] = INFINITY;
        if (out_parent) out_parent[i] = -1;
    }
    out_dist[src] = 0.0;
    for (int count = 0; count < n - 1; ++count) {
        double min_val = INFINITY;
        int u = -1;
        for (int v = 0; v < n; ++v) {
            if (!visited[v] && out_dist[v] <= min_val) {
                min_val = out_dist[v];
                u = v;
            }
        }
        if (u == -1) break;
        visited[u] = true;
        for (int v = 0; v < n; ++v) {
            double weight = adj[u * n + v];
            if (c) c->comparisons++;
            if (!visited[v] && isfinite(weight) && out_dist[u] + weight < out_dist[v]) {
                out_dist[v] = out_dist[u] + weight;
                if (out_parent) out_parent[v] = u;
            }
        }
    }
}
double libraa_prim_mst(int n, const double* adj, int* out_parent, Counters* c) {
    if(n<1||n>128||!adj){
        errno=EINVAL;
        return NAN;
    }
    for(int i=0;i<n;++i)for(int j=0;j<n;++j)if(adj[i*n+j]!=adj[j*n+i]){
        errno=EINVAL;
        return NAN;
    }
    double key[128];
    bool in_mst[128] = {
        false
    };
    for (int i = 0; i < n; ++i) {
        key[i] = INFINITY;
        if (out_parent) out_parent[i] = -1;
    }
    key[0] = 0.0;
    double total_weight = 0.0;
    for (int count = 0; count < n; ++count) {
        double min_val = INFINITY;
        int u = -1;
        for (int v = 0; v < n; ++v) {
            if (!in_mst[v] && key[v] < min_val) {
                min_val = key[v];
                u = v;
            }
        }
        if (u == -1 || !isfinite(key[u])) {
            errno=EINVAL;
            return NAN;
        }
        in_mst[u] = true;
        total_weight += key[u];
        for (int v = 0; v < n; ++v) {
            double weight = adj[u * n + v];
            if (c) c->comparisons++;
            if (isfinite(weight) && !in_mst[v] && weight < key[v]) {
                key[v] = weight;
                if (out_parent) out_parent[v] = u;
            }
        }
    }
    return total_weight;
}
// Disjoint Set for Kruskal
static int find_set(int parent[], int i) {
    if (parent[i] == i) return i;
    return parent[i] = find_set(parent, parent[i]);
}
double libraa_kruskal_mst(int n, int edge_count, const int* u, const int* v, const double* w, Counters* c) {
    if(n<1||n>128||edge_count<0||edge_count>512||!u||!v||!w){
        errno=EINVAL;
        return NAN;
    }
    for(int i=0;i<edge_count;++i)if(u[i]<0||u[i]>=n||v[i]<0||v[i]>=n){
        errno=EINVAL;
        return NAN;
    }
    // Sort edges
    int indices[512];
    for (int i = 0; i < edge_count; ++i) indices[i] = i;
    for (int i = 0; i < edge_count - 1; ++i) {
        for (int j = 0; j < edge_count - i - 1; ++j) {
            if (c) c->comparisons++;
            if (w[indices[j]] > w[indices[j + 1]]) {
                int tmp = indices[j];
                indices[j] = indices[j + 1];
                indices[j + 1] = tmp;
            }
        }
    }
    int parent[128];
    for (int i = 0; i < n; ++i) parent[i] = i;
    double total_weight = 0.0;
    int edges_taken = 0;
    for (int i = 0; i < edge_count && edges_taken < n - 1; ++i) {
        int e = indices[i];
        int set_u = find_set(parent, u[e]);
        int set_v = find_set(parent, v[e]);
        if (c) c->comparisons++;
        if (set_u != set_v) {
            total_weight += w[e];
            parent[set_u] = set_v;
            edges_taken++;
        }
    }
    if(edges_taken!=n-1){
        errno=EINVAL;
        return NAN;
    }
    return total_weight;
}
// ============================================================================
// 3. DYNAMIC PROGRAMMING
// ============================================================================
int libraa_knapsack_01_dp(int n,int W,const int*wt,const int*val,int*selected,Counters*c){
    if(n<0||n>100||W<0||W>10000||(n&&(!wt||!val))){
        errno=EINVAL;
        return -1;
    }
    for(int i=0;i<n;++i)if(wt[i]<1||val[i]<0||val[i]>1000000){
        errno=EINVAL;
        return -1;
    }
    size_t width=(size_t)W+1;
    int*dp=calloc(((size_t)n+1)*width,sizeof(int));
    if(!dp){
        errno=ENOMEM;
        return -1;
    }
    counters_memory(c,((size_t)n+1)*width*sizeof(int));
    for(int i=1;i<=n;++i)for(int w=0;w<=W;++w){
        int best=dp[(i-1)*width+w];
        if(c)c->comparisons++;
        if(wt[i-1]<=w){
            int take=val[i-1]+dp[(i-1)*width+w-wt[i-1]];
            if(take>best)best=take;
        }
        dp[i*width+w]=best;
    }
    int result=dp[n*width+W],w=W;
    if(selected){
        memset(selected,0,(size_t)n*sizeof(int));
        for(int i=n;i>0;--i)if(dp[i*width+w]!=dp[(i-1)*width+w]){
            selected[i-1]=1;
            w-=wt[i-1];
        }
    }
    free(dp);
    return result;
}
int libraa_matrix_chain_order(const int* p, int n, Counters* c) {
    if(!p||n<1||n>30){
        errno=EINVAL;
        return -1;
    }
    for(int i=0;i<=n;++i)if(p[i]<1||p[i]>100){
        errno=EINVAL;
        return -1;
    }
    // p has size n + 1 matrices
    int m[32][32] = {
        {
            0
        }
    };
    for (int L = 2; L <= n; ++L) {
        for (int i = 1; i <= n - L + 1; ++i) {
            int j = i + L - 1;
            m[i][j] = 1000000000;
            for (int k = i; k <= j - 1; ++k) {
                int q = m[i][k] + m[k + 1][j] + p[i - 1] * p[k] * p[j];
                if (c) {
                    c->comparisons++;
                    c->hash_ops += 2;
                }
                if (q < m[i][j]) {
                    m[i][j] = q;
                }
            }
        }
    }
    return m[1][n];
}
double libraa_optimal_bst_roots(const double* p, const double* q, int n, int* roots, Counters* c) {
    if(!p||!q||n<0||n>30){
        errno=EINVAL;
        return NAN;
    }
    if(roots)for(int i=0;i<(n+1)*(n+1);++i)roots[i]=-1;
    for(int i=0;i<n;++i)if(p[i]<0||!isfinite(p[i])){
        errno=EINVAL;
        return NAN;
    }
    for(int i=0;i<=n;++i)if(q[i]<0||!isfinite(q[i])){
        errno=EINVAL;
        return NAN;
    }
    double e[32][32] = {
        {
            0
        }
    };
    double w[32][32] = {
        {
            0
        }
    };
    for (int i = 1; i <= n + 1; ++i) {
        e[i][i - 1] = q[i - 1];
        w[i][i - 1] = q[i - 1];
    }
    for (int l = 1; l <= n; ++l) {
        for (int i = 1; i <= n - l + 1; ++i) {
            int j = i + l - 1;
            e[i][j] = INFINITY;
            w[i][j] = w[i][j - 1] + p[j - 1] + q[j];
            for (int r = i; r <= j; ++r) {
                double t = e[i][r - 1] + e[r + 1][j] + w[i][j];
                if (c) c->comparisons++;
                /* Decimal probabilities have tiny representation differences across
                   SSE/x87 compilers. Treat relative differences <=1e-12 as
                   a tie; keep the first (lowest-index) root. */
                double tolerance=1e-12*fmax(fabs(t),fabs(e[i][j]));
                if (!isfinite(e[i][j]) || t < e[i][j]-tolerance) {
                    e[i][j] = t;
                    if(roots)roots[i*(n+1)+j]=r;
                }
            }
        }
    }
    return e[1][n];
}
double libraa_optimal_bst(const double*p,const double*q,int n,Counters*c){
    return libraa_optimal_bst_roots(p,q,n,NULL,c);
}
bool libraa_bellman_ford(int n, int m, const int* u, const int* v, const double* w, int src, double* out_dist, Counters* c) {
    if(n<1||n>128||m<0||src<0||src>=n||!out_dist||!u||!v||!w){
        errno=EINVAL;
        return false;
    }
    for(int i=0;i<m;++i)if(u[i]<0||u[i]>=n||v[i]<0||v[i]>=n){
        errno=EINVAL;
        return false;
    }
    for (int i = 0; i < n; ++i) out_dist[i] = INFINITY;
    out_dist[src] = 0.0;
    for (int i = 1; i <= n - 1; ++i) {
        for (int j = 0; j < m; ++j) {
            if (isfinite(out_dist[u[j]])) {
                if (c) c->comparisons++;
                if (out_dist[u[j]] + w[j] < out_dist[v[j]]) {
                    out_dist[v[j]] = out_dist[u[j]] + w[j];
                }
            }
        }
    }
    // Check negative-weight cycles
    for (int j = 0; j < m; ++j) {
        if (isfinite(out_dist[u[j]])) {
            if (out_dist[u[j]] + w[j] < out_dist[v[j]]) {
                return false;
                // Negative cycle detected
            }
        }
    }
    return true;
}
void libraa_floyd_warshall(int n, const double* adj, double* out_dist, Counters* c) {
    if(n<1||n>128||!adj||!out_dist){
        errno=EINVAL;
        return;
    }
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            out_dist[i * n + j] = adj[i * n + j];
        }
    }
    for (int k = 0; k < n; ++k) {
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (isfinite(out_dist[i*n+k]) && isfinite(out_dist[k*n+j])) {
                    double via_k = out_dist[i * n + k] + out_dist[k * n + j];
                    if (c) c->comparisons++;
                    if (via_k < out_dist[i * n + j]) {
                        out_dist[i * n + j] = via_k;
                    }
                }
            }
        }
    }
    for(int i=0;i<n;++i)if(out_dist[i*n+i]<0){errno=EDOM;return;}
}
double libraa_tsp_dp(int n,const double*g,int*path,Counters*c){
    if(n<1||n>12||!g){
        errno=EINVAL;
        return NAN;
    }
    if(n==1){
        if(path){
            path[0]=path[1]=0;
        }
        return 0;
    }
    int states=1<<n,all=states-1;
    size_t size=(size_t)states*n;
    double*dp=malloc(size*sizeof(double));
    int*next=malloc(size*sizeof(int));
    if(!dp||!next){
        free(dp);
        free(next);
        errno=ENOMEM;
        return NAN;
    }
    for(size_t i=0;i<size;++i){
        dp[i]=INFINITY;
        next[i]=-1;
    }
    for(int u=0;u<n;++u)dp[all*n+u]=g[u*n];
    for(int mask=all-1;mask>=1;--mask)for(int u=0;u<n;++u)if(mask&(1<<u))for(int v=0;v<n;++v)if(!(mask&(1<<v))){
        double cost=g[u*n+v]+dp[(mask|(1<<v))*n+v];
        if(c)c->comparisons++;
        if(cost<dp[mask*n+u]){
            dp[mask*n+u]=cost;
            next[mask*n+u]=v;
        }
    }
    double result=dp[n];
    if(path){
        for(int i=0;i<=n;++i)path[i]=-1;
        if(isfinite(result)){
            int mask=1,u=0;
            path[0]=0;
            for(int i=1;i<n;++i){
                u=next[mask*n+u];
                path[i]=u;
                mask|=1<<u;
            }
            path[n]=0;
        }
    }
    counters_memory(c,size*(sizeof(double)+sizeof(int)));
    free(dp);
    free(next);
    return result;
}
// ============================================================================
// 4. BACKTRACKING & BRANCH AND BOUND
// ============================================================================
static bool is_safe_nqueens(const int* board, int row, int col) {
    for (int i = 0; i < row; ++i) {
        if (board[i] == col || abs(board[i] - col) == abs(i - row)) {
            return false;
        }
    }
    return true;
}
static bool solve_nqueens_rec(int* board, int row, int n, Counters* c) {
    if (row == n) return true;
    for (int col = 0; col < n; ++col) {
        if (c) c->comparisons++;
        if (is_safe_nqueens(board, row, col)) {
            board[row] = col;
            if (solve_nqueens_rec(board, row + 1, n, c)) {
                return true;
            }
        }
    }
    return false;
}
int libraa_nqueens(int n, int* out_first_solution, Counters* c) {
    if(n<1||n>12||!out_first_solution){
        errno=EINVAL;
        return -1;
    }
    int board[32];
    for (int i = 0; i < n; ++i) board[i] = -1;
    bool solved = solve_nqueens_rec(board, 0, n, c);
    if (solved && out_first_solution) {
        for (int i = 0; i < n; ++i) {
            out_first_solution[i] = board[i];
        }
        return 1;
    }
    return 0;
}
static bool subset_sum_rec(const int* set, int n, int idx, int curr_sum, int target_sum, int* sol, Counters* c) {
    if (curr_sum == target_sum) return true;
    if (idx >= n || curr_sum > target_sum) return false;
    if (c) c->comparisons++;
    sol[idx] = 1;
    if (subset_sum_rec(set, n, idx + 1, curr_sum + set[idx], target_sum, sol, c)) {
        return true;
    }
    sol[idx] = 0;
    return subset_sum_rec(set, n, idx + 1, curr_sum, target_sum, sol, c);
}
int libraa_sum_of_subsets(const int* set, int n, int target_sum, int* out_solution, Counters* c) {
    if(n<0||n>24||target_sum<0||!out_solution||!set){
        errno=EINVAL;
        return -1;
    }
    for(int i=0;i<n;++i)if(set[i]<0||set[i]>1000000){
        errno=EINVAL;
        return -1;
    }
    for (int i = 0; i < n; ++i) out_solution[i] = 0;
    return subset_sum_rec(set, n, 0, 0, target_sum, out_solution, c) ? 1 : 0;
}
static void knapsack_bt(int idx, int curr_w, int curr_v, int n, int W,                         const int* wt, const int* val, int* best_v,                         int* curr_sol, int* best_sol, Counters* c) {
    if (curr_w > W) return;
    if (c) c->comparisons++;
    if (curr_v > *best_v) {
        *best_v = curr_v;
        for (int i = 0; i < n; ++i) best_sol[i] = curr_sol[i];
    }
    if (idx >= n) return;
    // Branch 1: include item idx
    curr_sol[idx] = 1;
    knapsack_bt(idx + 1, curr_w + wt[idx], curr_v + val[idx], n, W, wt, val, best_v, curr_sol, best_sol, c);
    // Branch 2: exclude item idx
    curr_sol[idx] = 0;
    knapsack_bt(idx + 1, curr_w, curr_v, n, W, wt, val, best_v, curr_sol, best_sol, c);
}
int libraa_knapsack_01_backtrack(int n, int W, const int* wt, const int* val, int* out_selected, Counters* c) {
    if(n<0||n>20||W<0||W>10000||(n&&(!wt||!val))){
        errno=EINVAL;
        return -1;
    }
    for(int i=0;i<n;++i)if(wt[i]<1||val[i]<0||val[i]>1000000){
        errno=EINVAL;
        return -1;
    }
    if(out_selected)memset(out_selected,0,(size_t)n*sizeof(int));
    counters_memory(c,2*64*sizeof(int)+(size_t)(n+1)*8*sizeof(int));
    int curr_sol[64] = {
        0
    };
    int best_sol[64] = {
        0
    };
    int best_v = 0;
    knapsack_bt(0, 0, 0, n, W, wt, val, &best_v, curr_sol, best_sol, c);
    if (out_selected) {
        for (int i = 0; i < n; ++i) out_selected[i] = best_sol[i];
    }
    return best_v;
}
typedef struct {
    int id;
    int weight;
    int value;
    double ratio;
}
BBItem;
typedef struct {
    int level;
    int weight;
    int value;
    double bound;
    int take[64];
}
BBNode;
static double calc_bound(const BBNode* u, int n, int W, const BBItem* items, Counters* c) {
    if (u->weight >= W) return 0.0;
    double profit_bound = (double)u->value;
    int total_w = u->weight;
    int j = u->level;
    while (j < n && total_w + items[j].weight <= W) {
        total_w += items[j].weight;
        profit_bound += items[j].value;
        if (c) c->comparisons++;
        j++;
    }
    if (j < n) {
        if (c) c->comparisons++;
        profit_bound += (double)(W - total_w) * items[j].ratio;
    }
    return profit_bound;
}
static void sort_bb_items(BBItem* items, int n, Counters* c) {
    for (int i = 0; i < n - 1; ++i) {
        for (int j = 0; j < n - i - 1; ++j) {
            if (c) c->comparisons++;
            if (items[j].ratio < items[j + 1].ratio) {
                BBItem tmp = items[j];
                items[j] = items[j + 1];
                items[j + 1] = tmp;
            }
        }
    }
}
// 4a. FIFO Branch-and-Bound (Queue-based)
int libraa_knapsack_01_fifo_bb(int n, int W, const int* wt, const int* val, int* out_selected, Counters* c) {
    if(n<0||n>20||W<0||W>10000||(n&&(!wt||!val))){
        errno=EINVAL;
        return -1;
    }
    for(int i=0;i<n;++i)if(wt[i]<1||val[i]<0||val[i]>1000000){
        errno=EINVAL;
        return -1;
    }
    if(out_selected)memset(out_selected,0,(size_t)n*sizeof(int));
    if (n <= 0 || W <= 0 || !wt || !val) return 0;
    int cap = n > 64 ? 64 : n;
    BBItem items[64];
    for (int i = 0; i < cap; ++i) {
        items[i].id = i;
        items[i].weight = wt[i];
        items[i].value = val[i];
        items[i].ratio = (wt[i] > 0) ? ((double)val[i] / (double)wt[i]) : 0.0;
    }
    sort_bb_items(items, cap, c);
    static BBNode queue[8192];
    counters_memory(c,sizeof(queue)+sizeof(BBItem)*64+sizeof(int)*64);
    int head = 0, tail = 0;
    BBNode root;
    memset(&root, 0, sizeof(root));
    root.bound = calc_bound(&root, cap, W, items, c);
    queue[tail++] = root;
    int max_profit = 0;
    int best_take[64] = {
        0
    };
    while (head < tail) {
        BBNode u = queue[head++];
        if (c) c->comparisons++;
        if (u.bound <= (double)max_profit || u.level >= cap) continue;
        // Branch 1: include items[u.level]
        if (u.weight + items[u.level].weight <= W) {
            BBNode v1 = u;
            v1.level = u.level + 1;
            v1.weight += items[u.level].weight;
            v1.value += items[u.level].value;
            v1.take[items[u.level].id] = 1;
            if (c) c->comparisons++;
            if (v1.value > max_profit) {
                max_profit = v1.value;
                for (int i = 0; i < cap; ++i) best_take[i] = v1.take[i];
            }
            v1.bound = calc_bound(&v1, cap, W, items, c);
            if (v1.bound > (double)max_profit && v1.level < cap) {
                if(tail>=8192){
                    errno=E2BIG;
                    return -1;
                }
                queue[tail++] = v1;
            }
        }
        // Branch 2: exclude items[u.level]
        BBNode v0 = u;
        v0.level = u.level + 1;
        v0.take[items[u.level].id] = 0;
        v0.bound = calc_bound(&v0, cap, W, items, c);
        if (v0.bound > (double)max_profit && v0.level < cap) {
            if(tail>=8192){
                errno=E2BIG;
                return -1;
            }
            queue[tail++] = v0;
        }
    }
    if (out_selected) {
        for (int i = 0; i < cap; ++i) out_selected[i] = best_take[i];
    }
    return max_profit;
}
// 4b. LIFO Branch-and-Bound (Stack-based)
int libraa_knapsack_01_lifo_bb(int n, int W, const int* wt, const int* val, int* out_selected, Counters* c) {
    if(n<0||n>20||W<0||W>10000||(n&&(!wt||!val))){
        errno=EINVAL;
        return -1;
    }
    for(int i=0;i<n;++i)if(wt[i]<1||val[i]<0||val[i]>1000000){
        errno=EINVAL;
        return -1;
    }
    if(out_selected)memset(out_selected,0,(size_t)n*sizeof(int));
    if (n <= 0 || W <= 0 || !wt || !val) return 0;
    int cap = n > 64 ? 64 : n;
    BBItem items[64];
    for (int i = 0; i < cap; ++i) {
        items[i].id = i;
        items[i].weight = wt[i];
        items[i].value = val[i];
        items[i].ratio = (wt[i] > 0) ? ((double)val[i] / (double)wt[i]) : 0.0;
    }
    sort_bb_items(items, cap, c);
    static BBNode stack[8192];
    counters_memory(c,sizeof(stack)+sizeof(BBItem)*64+sizeof(int)*64);
    int top = 0;
    BBNode root;
    memset(&root, 0, sizeof(root));
    root.bound = calc_bound(&root, cap, W, items, c);
    stack[top++] = root;
    int max_profit = 0;
    int best_take[64] = {
        0
    };
    while (top > 0) {
        BBNode u = stack[--top];
        if (c) c->comparisons++;
        if (u.bound <= (double)max_profit || u.level >= cap) continue;
        // Exclude branch (push first so include is popped first)
        BBNode v0 = u;
        v0.level = u.level + 1;
        v0.take[items[u.level].id] = 0;
        v0.bound = calc_bound(&v0, cap, W, items, c);
        if (v0.bound > (double)max_profit && v0.level < cap) {
            if(top>=8192){
                errno=E2BIG;
                return -1;
            }
            stack[top++] = v0;
        }
        // Include branch
        if (u.weight + items[u.level].weight <= W) {
            BBNode v1 = u;
            v1.level = u.level + 1;
            v1.weight += items[u.level].weight;
            v1.value += items[u.level].value;
            v1.take[items[u.level].id] = 1;
            if (c) c->comparisons++;
            if (v1.value > max_profit) {
                max_profit = v1.value;
                for (int i = 0; i < cap; ++i) best_take[i] = v1.take[i];
            }
            v1.bound = calc_bound(&v1, cap, W, items, c);
            if (v1.bound > (double)max_profit && v1.level < cap) {
                if(top>=8192){
                    errno=E2BIG;
                    return -1;
                }
                stack[top++] = v1;
            }
        }
    }
    if (out_selected) {
        for (int i = 0; i < cap; ++i) out_selected[i] = best_take[i];
    }
    return max_profit;
}
// 4c. Least-Cost / Best-First Branch-and-Bound (Priority Queue)
static void heap_push(BBNode* heap, int* size, BBNode node, Counters* c) {
    int i = (*size)++;
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (c) c->comparisons++;
        if (heap[parent].bound < node.bound) {
            heap[i] = heap[parent];
            i = parent;
        }
        else {
            break;
        }
    }
    heap[i] = node;
}
static BBNode heap_pop(BBNode* heap, int* size, Counters* c) {
    BBNode top = heap[0];
    BBNode last = heap[--(*size)];
    if (*size > 0) {
        int i = 0;
        while (2 * i + 1 < *size) {
            int left = 2 * i + 1;
            int right = 2 * i + 2;
            int best = left;
            if (right < *size) {
                if (c) c->comparisons++;
                if (heap[right].bound > heap[left].bound) best = right;
            }
            if (c) c->comparisons++;
            if (last.bound < heap[best].bound) {
                heap[i] = heap[best];
                i = best;
            }
            else {
                break;
            }
        }
        heap[i] = last;
    }
    return top;
}
int libraa_knapsack_01_least_cost_bb(int n, int W, const int* wt, const int* val, int* out_selected, Counters* c) {
    if(n<0||n>20||W<0||W>10000||(n&&(!wt||!val))){
        errno=EINVAL;
        return -1;
    }
    for(int i=0;i<n;++i)if(wt[i]<1||val[i]<0||val[i]>1000000){
        errno=EINVAL;
        return -1;
    }
    if(out_selected)memset(out_selected,0,(size_t)n*sizeof(int));
    if (n <= 0 || W <= 0 || !wt || !val) return 0;
    int cap = n > 64 ? 64 : n;
    BBItem items[64];
    for (int i = 0; i < cap; ++i) {
        items[i].id = i;
        items[i].weight = wt[i];
        items[i].value = val[i];
        items[i].ratio = (wt[i] > 0) ? ((double)val[i] / (double)wt[i]) : 0.0;
    }
    sort_bb_items(items, cap, c);
    static BBNode heap[8192];
    counters_memory(c,sizeof(heap)+sizeof(BBItem)*64+sizeof(int)*64);
    int heap_size = 0;
    BBNode root;
    memset(&root, 0, sizeof(root));
    root.bound = calc_bound(&root, cap, W, items, c);
    heap_push(heap, &heap_size, root, c);
    int max_profit = 0;
    int best_take[64] = {
        0
    };
    while (heap_size > 0) {
        BBNode u = heap_pop(heap, &heap_size, c);
        if (c) c->comparisons++;
        // If highest bound in heap cannot beat max_profit, optimal solution is guaranteed!
        if (u.bound <= (double)max_profit) break;
        if (u.level >= cap) continue;
        // Include branch
        if (u.weight + items[u.level].weight <= W) {
            BBNode v1 = u;
            v1.level = u.level + 1;
            v1.weight += items[u.level].weight;
            v1.value += items[u.level].value;
            v1.take[items[u.level].id] = 1;
            if (c) c->comparisons++;
            if (v1.value > max_profit) {
                max_profit = v1.value;
                for (int i = 0; i < cap; ++i) best_take[i] = v1.take[i];
            }
            v1.bound = calc_bound(&v1, cap, W, items, c);
            if (v1.bound > (double)max_profit && v1.level < cap) {
                if(heap_size>=8192){
                    errno=E2BIG;
                    return -1;
                }
                heap_push(heap, &heap_size, v1, c);
            }
        }
        // Exclude branch
        BBNode v0 = u;
        v0.level = u.level + 1;
        v0.take[items[u.level].id] = 0;
        v0.bound = calc_bound(&v0, cap, W, items, c);
        if (v0.bound > (double)max_profit && v0.level < cap) {
            if(heap_size>=8192){
                errno=E2BIG;
                return -1;
            }
            heap_push(heap, &heap_size, v0, c);
        }
    }
    if (out_selected) {
        for (int i = 0; i < cap; ++i) out_selected[i] = best_take[i];
    }
    return max_profit;
}
int libraa_knapsack_01_branch_and_bound(int n, int W, const int* wt, const int* val, int* out_selected, Counters* c) {
    // Defaults to Least-Cost (Best-First) Branch-and-Bound
    return libraa_knapsack_01_least_cost_bb(n, W, wt, val, out_selected, c);
}
// ============================================================================
// 5. FLOW AND MATCHING
// ============================================================================
static bool bfs_flow(int n, const int* r_graph, int s, int t, int* parent, Counters* c) {
    bool visited[64] = {
        false
    };
    int queue[64];
    int head = 0, tail = 0;
    queue[tail++] = s;
    visited[s] = true;
    parent[s] = -1;
    while (head < tail) {
        int u = queue[head++];
        for (int v = 0; v < n; ++v) {
            if (c) c->comparisons++;
            if (!visited[v] && r_graph[u * n + v] > 0) {
                if (v == t) {
                    parent[v] = u;
                    return true;
                }
                queue[tail++] = v;
                visited[v] = true;
                parent[v] = u;
            }
        }
    }
    return false;
}
int libraa_ford_fulkerson(int n, const int* cap_matrix, int src, int sink, Counters* c) {
    counters_memory(c,64*64*sizeof(int)+4*64*sizeof(int));
    if(n<2||n>64||!cap_matrix||src<0||sink<0||src>=n||sink>=n||src==sink){
        errno=EINVAL;
        return -1;
    }
    for(int i=0;i<n*n;++i)if(cap_matrix[i]<0||cap_matrix[i]>1000000){
        errno=EINVAL;
        return -1;
    }
    int r_graph[64 * 64];
    for (int i = 0; i < n * n; ++i) {
        r_graph[i] = cap_matrix[i];
    }
    int parent[64];
    int max_flow = 0;
    while (bfs_flow(n, r_graph, src, sink, parent, c)) {
        int path_flow = 1000000000;
        for (int v = sink; v != src; v = parent[v]) {
            int u = parent[v];
            if (r_graph[u * n + v] < path_flow) {
                path_flow = r_graph[u * n + v];
            }
        }
        for (int v = sink; v != src; v = parent[v]) {
            int u = parent[v];
            r_graph[u * n + v] -= path_flow;
            r_graph[v * n + u] += path_flow;
        }
        max_flow += path_flow;
    }
    return max_flow;
}
static bool bpm_dfs(int u, int right_count, const int* adj, bool* seen, int* matchR, Counters* c) {
    for (int v = 0; v < right_count; ++v) {
        if (c) c->comparisons++;
        if (adj[u * right_count + v] && !seen[v]) {
            seen[v] = true;
            if (matchR[v] < 0 || bpm_dfs(matchR[v], right_count, adj, seen, matchR, c)) {
                matchR[v] = u;
                return true;
            }
        }
    }
    return false;
}
int libraa_max_bipartite_matching(int left_count, int right_count, const int* adj, int* out_match, Counters* c) {
    counters_memory(c,64*(sizeof(int)+sizeof(bool)));
    if(left_count<0||right_count<0||left_count>64||right_count>64||!adj){
        errno=EINVAL;
        return -1;
    }
    int matchR[64];
    for (int i = 0; i < right_count; ++i) matchR[i] = -1;
    int max_matches = 0;
    for (int u = 0; u < left_count; ++u) {
        bool seen[64] = {
            false
        };
        if (bpm_dfs(u, right_count, adj, seen, matchR, c)) {
            max_matches++;
        }
    }
    if (out_match) {
        for (int v = 0; v < right_count; ++v) {
            out_match[v] = matchR[v];
        }
    }
    return max_matches;
}
// ============================================================================
// 6. NUMBER THEORY (CHINESE REMAINDER THEOREM)
// ============================================================================
static long long extended_gcd(long long a, long long b, long long* x, long long* y) {
    if (a == 0) {
        *x = 0;
        *y = 1;
        return b;
    }
    long long x1, y1;
    long long gcd = extended_gcd(b % a, a, &x1, &y1);
    *x = y1 - (b / a) * x1;
    *y = x1;
    return gcd;
}
static long long mod_inverse(long long a, long long m) {
    long long x, y;
    long long g = extended_gcd(a, m, &x, &y);
    if (g != 1) return -1;
    return (x % m + m) % m;
}
long long libraa_chinese_remainder_theorem(const long long*num,const long long*rem,int k,Counters*c){
    if(!num||!rem||k<1){
        errno=EINVAL;
        return -1;
    }
    long long prod=1,result=0;
    for(int i=0;i<k;++i){
        if(num[i]<2||prod>1000000000LL/num[i]){
            errno=EINVAL;
            return -1;
        }
        prod*=num[i];
    }
    for(int i=0;i<k;++i){
        long long pp=prod/num[i],inv=mod_inverse(pp,num[i]);
        if(inv<0){
            errno=EINVAL;
            return -1;
        }
        long long r=(rem[i]%num[i]+num[i])%num[i];
        result=(result+(r*pp%prod)*inv%prod)%prod;
        if(c)c->hash_ops+=3;
    }
    return result;
}
