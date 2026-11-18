#include "library.h"
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif
static int checks,failures;
#define CHECK(condition,label) do { ++checks; if(!(condition)){++failures;fprintf(out,"FAIL: %s (line %d)\n",label,__LINE__);} } while(0)
static unsigned state;
static unsigned random_number(void){
    state=state*1664525u+1013904223u;
    return state;
}
static int brute_knapsack(int n,int W,const int*wt,const int*val){
    int best=0;
    for(int mask=0;mask<(1<<n);++mask){
        int w=0,v=0;
        for(int i=0;i<n;++i)if(mask&(1<<i)){
            w+=wt[i];
            v+=val[i];
        }
        if(w<=W&&v>best)best=v;
    }
    return best;
}
static int brute_matching(int left,int right,const int*edges,int reader,int used){
    if(reader==left)return 0;
    int best=brute_matching(left,right,edges,reader+1,used);
    for(int j=0;j<right;++j)if(edges[reader*right+j]&&!(used&(1<<j))){
        int v=1+brute_matching(left,right,edges,reader+1,used|(1<<j));
        if(v>best)best=v;
    }
    return best;
}
static int feasible(int n,int W,const int*wt,const int*val,const int*chosen,int result){
    int w=0,v=0;
    for(int i=0;i<n;++i){
        if(chosen[i]!=0&&chosen[i]!=1)return 0;
        if(chosen[i]){
            w+=wt[i];
            v+=val[i];
        }
    }
    return v==result&&w<=W;
}
/* Verify the returned BST itself, independent of the DP cost table. */ static double bst_witness_cost(const double *p,const double *q,const int *roots,                               int lo,int hi,int depth,int stride) {
    if(lo>hi)return q[hi]*depth;
    int r=roots[lo*stride+hi];
    if(r<lo||r>hi)return NAN;
    return p[r-1]*depth         +bst_witness_cost(p,q,roots,lo,r-1,depth+1,stride)         +bst_witness_cost(p,q,roots,r+1,hi,depth+1,stride);
}
typedef struct {
    int priority,id;
    char payload[17];
}
SortExample;
static int compare_priority(const void *left,const void *right) {
    const SortExample *a=left,*b=right;
    return (a->priority>b->priority)-(a->priority<b->priority);
}
int run_tests(FILE*out){
    checks=failures=0;
    SortExample records[]={
        {
            2,0,"first"
        },{
            1,1,"second"
        },{
            2,2,"third"
        },{
            1,3,"fourth"
        }
    };
    CHECK(merge_sort(records,4,sizeof(*records),compare_priority,NULL),"shared merge supports records");
    CHECK(records[0].id==1&&records[1].id==3&&records[2].id==0&&records[3].id==2,"stable record sorting preserves ties and payload");
    CHECK(strcmp(records[0].payload,"second")==0&&strcmp(records[3].payload,"third")==0,"record bytes preserved");
    SortExample key={
        2,0,{
            0
        }
    };
    int found=binary_iter(records,4,sizeof(*records),&key,compare_priority,NULL);
    CHECK(found>=0&&records[found].priority==2,"shared binary search supports records");
    CHECK(quick_sort(records,4,sizeof(*records),compare_priority,NULL),"shared quick supports records");
    CHECK(records[0].priority==1&&records[3].priority==2,"record quick ordering");
    CHECK(!merge_sort(records,4,0,compare_priority,NULL),"zero-width sort rejected");
    state=17;
    for(int trial=0;trial<150;++trial){
        char text[65],pattern[12];
        int n=trial%64,m=trial%11;
        for(int i=0;i<n;++i)text[i]=(char)('a'+random_number()%3);
        text[n]='\0';
        for(int i=0;i<m;++i)pattern[i]=(char)('a'+random_number()%3);
        pattern[m]='\0';
        size_t oracle[65],actual[65],expected=match_text(text,pattern,NAIVE,oracle,65,NULL);
        for(int e=1;e<4;++e){
            size_t count=match_text(text,pattern,(Matcher)e,actual,65,NULL);
            CHECK(count==expected&&memcmp(oracle,actual,count*sizeof(size_t))==0,"matcher oracle positions");
        }
    }
    for(int e=0;e<4;++e){
        size_t p[3];
        CHECK(match_text("aaaa","aa",(Matcher)e,p,3,NULL)==3&&p[0]==0&&p[1]==1&&p[2]==2,"overlapping matches");
    }
    for(int n=0;n<80;++n){
        int a[80],b[80],counts[10]={
            0
        },after[10]={
            0
        };
        for(int i=0;i<n;++i){
            a[i]=b[i]=(int)(random_number()%10);
            ++counts[a[i]];
        }
        CHECK(merge_sort(a,n,sizeof(int),compare_ints,NULL),"merge succeeds");
        quick_sort(b,n,sizeof(int),compare_ints,NULL);
        CHECK(memcmp(a,b,(size_t)n*sizeof(int))==0,"sort agreement");
        for(int i=0;i<n;++i){
            ++after[a[i]];
            if(i)CHECK(a[i-1]<=a[i],"sorted order");
        }
        CHECK(memcmp(counts,after,sizeof(counts))==0,"multiset preserved");
        for(int key=-1;key<=10;++key){
            int x=binary_iter(a,n,sizeof(int),&key,compare_ints,NULL),y=binary_rec(a,n,sizeof(int),&key,compare_ints,NULL),present=0;
            for(int i=0;i<n;++i)if(a[i]==key)present=1;
            CHECK((x>=0)==present&&(y>=0)==present,"binary hit/miss oracle");
        }
    }
    char subseq[256];
    CHECK(lcs("data","dart",subseq,sizeof(subseq),NULL)==3&&strcmp(subseq,"dat")==0,"LCS reconstruction");
    CHECK(lcs("","",subseq,sizeof(subseq),NULL)==0,"empty LCS");
    CHECK(lcs("abc","abc",subseq,2,NULL)==-1,"small LCS buffer rejected");
    for(int trial=1;trial<60;++trial){
        int n=1+trial%15,a[16],best=-1000000,s,e;
        for(int i=0;i<n;++i)a[i]=(int)(random_number()%21)-10;
        for(int i=0;i<n;++i){
            int sum=0;
            for(int j=i;j<n;++j){
                sum+=a[j];
                if(sum>best)best=sum;
            }
        }
        CHECK(libraa_max_subarray_dc(a,0,n-1,&s,&e,NULL)==best,"subarray exhaustive oracle");
    }
    CHECK(libraa_fast_pow_mod(2,10,1000,NULL)==24&&libraa_fast_pow_mod(-2,3,5,NULL)==2&&libraa_fast_pow_mod(2,0,1,NULL)==0,"modular power");
    CHECK(libraa_fast_pow_mod(2,3,0,NULL)==-1,"invalid modulus");
    int celeb[]={
        0,1,1,0,0,1,0,0,0
    },none[]={
        0,0,0,0
    };
    CHECK(libraa_celebrity_find(celeb,3,NULL)==2&&libraa_celebrity_find(none,2,NULL)==-1,"celebrity verification");
    for(int n=1;n<=11;++n){
        double A[121],B[121],C[121],S[121];
        for(int i=0;i<n*n;++i){
            A[i]=random_number()%6;
            B[i]=random_number()%6;
        }
        errno=0;
        libraa_matrix_multiply_classical(n,A,B,C,NULL);
        libraa_matrix_multiply_strassen(n,A,B,S,NULL);
        CHECK(errno==0&&memcmp(C,S,(size_t)n*n*sizeof(double))==0,"padded recursive Strassen");
    }
    Activity slots[]={
        {
            1,1,4
        },{
            2,2,5
        },{
            3,4,7
        },{
            4,3,6
        }
    };
    int selected[4],room[4];
    CHECK(libraa_activity_selection(slots,4,selected,NULL)==2,"activity selection");
    CHECK(libraa_interval_partitioning(slots,4,room,NULL)==3,"partition optimum");
    for(int i=0;i<4;++i)for(int j=0;j<i;++j)if(room[i]==room[j])CHECK(slots[i].start>=slots[j].finish||slots[j].start>=slots[i].finish,"room feasibility");
    for(int trial=0;trial<100;++trial){
        int n=1+trial%9,W=(int)(random_number()%25),wt[9],val[9],take[9];
        for(int i=0;i<n;++i){
            wt[i]=1+(int)(random_number()%8);
            val[i]=(int)(random_number()%20);
        }
        int expected=brute_knapsack(n,W,wt,val);
        int result=libraa_knapsack_01_dp(n,W,wt,val,take,NULL);
        CHECK(result==expected&&feasible(n,W,wt,val,take,result),"DP exact witness");
        result=libraa_knapsack_01_bb(n,W,wt,val,take,BB_FIFO,NULL);
        CHECK(result==expected&&feasible(n,W,wt,val,take,result),"FIFO exact witness");
        result=libraa_knapsack_01_bb(n,W,wt,val,take,BB_LIFO,NULL);
        CHECK(result==expected&&feasible(n,W,wt,val,take,result),"LIFO exact witness");
        result=libraa_knapsack_01_bb(n,W,wt,val,take,BB_BEST_BOUND,NULL);
        CHECK(result==expected&&feasible(n,W,wt,val,take,result),"best-bound exact witness");
    }
    int invalid_wt[]={
        0
    },valid_val[]={
        5
    },take[1];
    CHECK(libraa_knapsack_01_dp(1,5,invalid_wt,valid_val,take,NULL)==-1,"invalid knapsack item");
    const char*texts[]={
        "","a","aaaa","abcd","hello library","\"quotes\" and commas,"
    };
    for(int i=0;i<6;++i){
        HuffmanTree h;
        char bits[8192],decoded[2049];
        int n=libraa_huffman_encode(texts[i],bits,sizeof(bits),&h,NULL);
        CHECK(n>=0&&libraa_huffman_decode(&h,bits,decoded,sizeof(decoded))>=0&&strcmp(texts[i],decoded)==0,"Huffman round trip");
        if(i==2)CHECK(n==4,"singleton Huffman payload");
        if(i==3)CHECK(n==8,"four-symbol Huffman payload");
    }
    double g[]={
        0,7,9,INFINITY,INFINITY,14,7,0,10,15,INFINITY,INFINITY,9,10,0,11,INFINITY,2,INFINITY,15,11,0,6,INFINITY,INFINITY,INFINITY,INFINITY,6,0,9,14,INFINITY,2,INFINITY,9,0
    };
    double all[36],dist[6],bf[6],ew[36];
    int u[36],v[36],parent[6],m=0;
    for(int i=0;i<6;++i)for(int j=0;j<6;++j)if(i!=j&&isfinite(g[i*6+j])){
        u[m]=i;
        v[m]=j;
        ew[m++]=g[i*6+j];
    }
    libraa_floyd_warshall(6,g,all,NULL);
    for(int src=0;src<6;++src){
        libraa_dijkstra(6,src,g,dist,parent,NULL);
        int valid=libraa_bellman_ford(6,m,u,v,ew,src,bf,NULL);
        for(int i=0;i<6;++i)CHECK(valid&&dist[i]==bf[i]&&dist[i]==all[src*6+i],"shortest-path cross-oracles");
    }
    CHECK(all[5]==11,"missing-edge regression");
    m=0;
    for(int i=0;i<6;++i)for(int j=i+1;j<6;++j)if(isfinite(g[i*6+j])){
        u[m]=i;
        v[m]=j;
        ew[m++]=g[i*6+j];
    }
    CHECK(libraa_prim_mst(6,g,parent,NULL)==33&&libraa_kruskal_mst(6,m,u,v,ew,NULL)==33,"MST cross-oracles");
    double zero[]={
        0,0,5,0,0,2,5,2,0
    };
    libraa_dijkstra(3,0,zero,dist,NULL,NULL);
    CHECK(dist[2]==2,"zero-cost edge");
    int nu[]={
        0,1
    },nv[]={
        1,0
    };
    double nw[]={
        -2,1
    };
    CHECK(!libraa_bellman_ford(2,2,nu,nv,nw,0,dist,NULL),"reachable negative cycle");
    double disconnected[]={
        0,INFINITY,INFINITY,0
    };
    CHECK(isnan(libraa_prim_mst(2,disconnected,NULL,NULL)),"disconnected MST rejected");
    int dims[]={
        10,20,30,40,30
    };
    CHECK(libraa_matrix_chain_order(dims,4,NULL)==30000,"matrix chain");
    double hit[]={
        .15,.10,.05,.10,.20
    },miss[]={
        .05,.10,.05,.05,.05,.10
    };
    int roots[36];
    double bst_cost=libraa_optimal_bst_roots(hit,miss,5,roots,NULL);
    CHECK(fabs(bst_cost-2.75)<1e-9,"OBST optimal cost");
    CHECK(fabs(bst_witness_cost(hit,miss,roots,1,5,1,6)-bst_cost)<1e-9,"OBST returned tree achieves reported cost");
    CHECK(roots[11]==2,"OBST near-equal ties keep the first root deterministically");
    for(int n=1;n<=12;++n){
        double d[144];
        int path[13],seen[12]={
            0
        };
        for(int i=0;i<n;++i)for(int j=0;j<n;++j)d[i*n+j]=i==j?0:1;
        double cost=libraa_tsp_dp(n,d,path,NULL),actual=0;
        int valid=1;
        for(int i=0;i<n;++i){
            if(path[i]<0||path[i]>=n){
                valid=0;
                break;
            }
            ++seen[path[i]];
            actual+=d[path[i]*n+path[i+1]];
        }
        for(int i=0;i<n;++i)if(seen[i]!=1)valid=0;
        CHECK(valid&&cost==actual&&path[n]==0,"TSP route/cost including n11/n12");
    }
    double tour[]={
        0,10,15,20,10,0,35,25,15,35,0,30,20,25,30,0
    };
    int route[5];
    double cost=libraa_tsp_dp(4,tour,route,NULL),actual=0;
    for(int i=0;i<4;++i)actual+=tour[route[i]*4+route[i+1]];
    CHECK(cost==80&&actual==80,"TSP reconstructed optimal route");
    CHECK(isnan(libraa_tsp_dp(13,tour,route,NULL)),"TSP size bound");
    int board[8];
    CHECK(libraa_nqueens(8,board,NULL)==1,"8 queens solved");
    for(int i=0;i<8;++i)for(int j=0;j<i;++j)CHECK(board[i]!=board[j]&&abs(board[i]-board[j])!=i-j,"queen constraints");
    CHECK(libraa_nqueens(2,board,NULL)==0,"no two-queen solution");
    int set[]={
        10,7,5,18,12,20,15
    },solution[7],sum=0;
    libraa_sum_of_subsets(set,7,35,solution,NULL);
    for(int i=0;i<7;++i)if(solution[i])sum+=set[i];
    CHECK(sum==35,"subset witness");
    int cap[]={
        0,10,10,0,0,0,2,8,0,0,0,9,0,0,0,0
    };
    CHECK(libraa_ford_fulkerson(4,cap,0,3,NULL)==17,"max-flow oracle");
    for(int trial=0;trial<40;++trial){
        int edges[12],owner[3],cap2[81]={
            0
        };
        for(int i=0;i<12;++i)edges[i]=(int)(random_number()%2);
        int expected=brute_matching(4,3,edges,0,0),count=libraa_max_bipartite_matching(4,3,edges,owner,NULL),used[4]={
            0
        };
        for(int j=0;j<3;++j)if(owner[j]>=0){
            CHECK(edges[owner[j]*3+j]&&!used[owner[j]],"matching feasible");
            used[owner[j]]=1;
        }
        for(int i=0;i<4;++i){
            cap2[i+1]=1;
            for(int j=0;j<3;++j)cap2[(i+1)*9+5+j]=edges[i*3+j];
        }
        for(int j=0;j<3;++j)cap2[(5+j)*9+8]=1;
        CHECK(count==expected&&libraa_ford_fulkerson(9,cap2,0,8,NULL)==expected,"matching/flow/exhaustive oracle");
    }
    int hard_wt[20],hard_val[20],hard_take[20];
    for(int i=0;i<20;++i){
        hard_wt[i]=2;
        hard_val[i]=3;
    }
    errno=0;
    int hard=libraa_knapsack_01_bb(20,19,hard_wt,hard_val,hard_take,BB_FIFO,NULL);
    CHECK(hard==-1&&errno==E2BIG,"FIFO exhaustion returns failure instead of a false optimum");
    double negative_cycle[]={
        0,-2,1,0
    },negative_result[4];
    errno=0;
    libraa_floyd_warshall(2,negative_cycle,negative_result,NULL);
    CHECK(errno==EDOM,"Floyd negative cycle reported");
    long long mods[]={
        3,5,7
    },rems[]={
        2,3,2
    },badmods[]={
        4,6
    };
    CHECK(libraa_chinese_remainder_theorem(mods,rems,3,NULL)==23,"CRT");
    CHECK(libraa_chinese_remainder_theorem(badmods,rems,2,NULL)==-1,"noncoprime CRT rejected");
    Library lib,copy;
    char error[256];
    library_init(&lib);
    library_init(&copy);
    CHECK(library_load(&lib,"data/books.csv",error,sizeof(error)),"sample catalog loads");
    if(lib.count){
        CHECK(find_book(&lib,1,0)==find_book(&lib,1,1)&&find_book(&lib,1,0)!=NULL&&!find_book(&lib,-1,0),"book binary lookup");
        int ids[10];
        CHECK(keyword_lookup(&lib,"sorting",ids,10)>0,"keyword index");
        Hit a[10],b[10];
        int na=search_books(&lib,"sorting",NAIVE,a,10,NULL),nb=search_books(&lib,"sorting",KMP,b,10,NULL);
        CHECK(na==nb&&na>0,"catalog search count");
        for(int i=0;i<na;++i)CHECK(a[i].id==b[i].id&&a[i].score==b[i].score,"same ranked results");
        CHECK(search_books(&lib,"   ",KMP,a,10,NULL)==-1,"empty query rejected");
        CHECK(recommend_books(&lib,1,a,10)>0,"recommendation candidates");
        make_directory("results");
        char folder[128],path[256];
        snprintf(folder,sizeof(folder),"results/test_tmp_%lu",(unsigned long)clock());
        CHECK(export_reports(&lib,folder),"CSV export");
        snprintf(path,sizeof(path),"%s/catalog.csv",folder);
        CHECK(library_load(&copy,path,error,sizeof(error))&&copy.count==lib.count,"CSV round trip");
        snprintf(path,sizeof(path),"%s/bad.csv",folder);
        FILE*file=fopen(path,"w");
        if(file){
            fprintf(file,"id,title,author,subject,keywords,description,copies,hours,value\n1,x,a,s,k,d,-1,2,3\n");
            fclose(file);
            int before=lib.count;
            CHECK(!library_load(&lib,path,error,sizeof(error))&&lib.count==before,"transactional invalid load");
        }
        else CHECK(0,"write invalid fixture");
        snprintf(path,sizeof(path),"%s/quotes.csv",folder);
        file=fopen(path,"w");
        if(file){
            fprintf(file,"id,title,author,subject,keywords,description,copies,hours,value\n99,\"a, \"\"quoted\"\" book\",a,s,k,d,1,2,3\n");
            fclose(file);
            CHECK(library_load(&copy,path,error,sizeof(error))&&strcmp(copy.books[0].title,"a, \"quoted\" book")==0,"CSV quoted fields");
        }
        else CHECK(0,"write quoted fixture");
        const char*files[]={
            "catalog.csv","index.csv","recommendations.csv","relationships.csv","bad.csv","quotes.csv"
        };
        for(int i=0;i<6;++i){
            snprintf(path,sizeof(path),"%s/%s",folder,files[i]);
            remove(path);
        }
#ifdef _WIN32
        _rmdir(folder);
#else
        rmdir(folder);
#endif
    }
    library_free(&lib);
    library_free(&copy);
    fprintf(out,"%s: %d assertions, %d failures. Seeded independent oracles, witnesses, edge cases and regression tests.\n",failures?"FAIL":"PASS",checks,failures);
    return failures==0;
}
