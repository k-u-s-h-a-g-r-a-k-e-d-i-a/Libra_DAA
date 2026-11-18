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
the algorithms remain ordinary C functions. */ int make_directory(const char *path) {
#ifdef _WIN32
    int status = _mkdir(path);
#else
    int status = mkdir(path, 0755);
#endif
    return status == 0 || errno == EEXIST;
}
void normalize(char *text) {
    size_t read, write=0;
    int was_space=1;
    for(read=0; text[read]; ++read) {
        unsigned char c=(unsigned char)text[read];
        if(c<128 && isspace(c)) {
            if(!was_space){
                text[write++]=' ';
                was_space=1;
            }
        }
        else if(c>=32) {
            text[write++]=(char)(c>='A'&&c<='Z'?c+32:c);
            was_space=0;
        }
    }
    if(write && text[write-1]==' ') --write;
    text[write]='\0';
}
void library_init(Library *lib) {
    memset(lib,0,sizeof(*lib));
}
void library_free(Library *lib) {
    free(lib->books);
    free(lib->index);
    library_init(lib);
}
/* Merge sorts for the three record types. No qsort or external sort is used. */ static void merge_books(Book *a,Book *tmp,int lo,int hi) {
    int mid,i,j,k;
    if(hi-lo<2)return;
    mid=lo+(hi-lo)/2;
    merge_books(a,tmp,lo,mid);
    merge_books(a,tmp,mid,hi);
    i=lo;
    j=mid;
    k=lo;
    while(i<mid&&j<hi)tmp[k++]=a[i].id<=a[j].id?a[i++]:a[j++];
    while(i<mid)tmp[k++]=a[i++];
    while(j<hi)tmp[k++]=a[j++];
    for(k=lo;k<hi;++k)a[k]=tmp[k];
}
static int entry_before(const IndexEntry *a,const IndexEntry *b) {
    int cmp=strcmp(a->word,b->word);
    return cmp<0 || (cmp==0 && a->book_id<=b->book_id);
}
static void merge_index(IndexEntry*a,IndexEntry*tmp,int lo,int hi) {
    int mid,i,j,k;
    if(hi-lo<2)return;
    mid=lo+(hi-lo)/2;
    merge_index(a,tmp,lo,mid);
    merge_index(a,tmp,mid,hi);
    i=lo;
    j=mid;
    k=lo;
    while(i<mid&&j<hi)tmp[k++]=entry_before(&a[i],&a[j])?a[i++]:a[j++];
    while(i<mid)tmp[k++]=a[i++];
    while(j<hi)tmp[k++]=a[j++];
    for(k=lo;k<hi;++k)a[k]=tmp[k];
}
static int hit_before(Hit a,Hit b) {
    return a.score>b.score || (a.score==b.score && a.id<=b.id);
}
static void merge_hits(Hit*a,Hit*tmp,int lo,int hi,Counters*s) {
    int mid,i,j,k;
    if(hi-lo<2)return;
    mid=lo+(hi-lo)/2;
    merge_hits(a,tmp,lo,mid,s);
    merge_hits(a,tmp,mid,hi,s);
    i=lo;
    j=mid;
    k=lo;
    while(i<mid&&j<hi){
        if(s)++s->comparisons;
        tmp[k++]=hit_before(a[i],a[j])?a[i++]:a[j++];
    }
    while(i<mid)tmp[k++]=a[i++];
    while(j<hi)tmp[k++]=a[j++];
    for(k=lo;k<hi;++k)a[k]=tmp[k];
}
static int rank_hits(Hit*a,int n,Counters*s){
    if(n<2)return 1;
    Hit*tmp=malloc((size_t)n*sizeof(*tmp));
    if(!tmp)return 0;
    counters_memory(s,(size_t)n*sizeof(*tmp));
    merge_hits(a,tmp,0,n,s);
    free(tmp);
    return 1;
}
static int csv_fields(const char *line,char fields[9][2050]) {
    int field=0,quoted=0,closed=0;
    size_t at=0;
    memset(fields,0,9*2050);
    for(size_t i=0;line[i]&&line[i]!='\n'&&line[i]!='\r';++i){
        char ch=line[i];
        if(quoted){
            if(ch=='"'){
                if(line[i+1]=='"'){
                    if(at>=2049)return 0;
                    fields[field][at++]='"';
                    ++i;
                }
                else{
                    quoted=0;
                    closed=1;
                }
            }
            else {
                if(at>=2049)return 0;
                fields[field][at++]=ch;
            }
        }
        else if(ch==','){
            fields[field][at]='\0';
            if(++field>=9)return 0;
            at=0;
            closed=0;
        }
        else if(ch=='"'){
            if(at||closed)return 0;
            quoted=1;
        }
        else {
            if(closed||at>=2049)return 0;
            fields[field][at++]=ch;
        }
    }
    fields[field][at]='\0';
    return !quoted&&field==8;
}
static int parse_int(const char*s,int*out){
    char*end;
    long v;
    errno=0;
    v=strtol(s,&end,10);
    if(errno||end==s||*end||v<INT_MIN||v>INT_MAX)return 0;
    *out=(int)v;
    return 1;
}
static int copy_text(char *out,size_t cap,const char*in){
    if(strlen(in)>=cap)return 0;
    strcpy(out,in);
    return 1;
}
static int build_index(Library*lib){
    int count=0;
    for(int i=0;i<lib->count;++i)count+=lib->books[i].keyword_count;
    IndexEntry *entries=malloc((size_t)count*sizeof(*entries)),*tmp=malloc((size_t)count*sizeof(*tmp));
    if(!entries||!tmp){
        free(entries);
        free(tmp);
        return 0;
    }
    int at=0;
    for(int i=0;i<lib->count;++i)for(int k=0;k<lib->books[i].keyword_count;++k){
        strcpy(entries[at].word,lib->books[i].keywords[k]);
        entries[at++].book_id=lib->books[i].id;
    }
    merge_index(entries,tmp,0,count);
    free(tmp);
    lib->index=entries;
    lib->index_count=count;
    return 1;
}
int library_load(Library*lib,const char*path,char*error,size_t cap){
    FILE *file=fopen(path,"r");
    Library fresh;
    char line[8194];
    int row=1,ok=0,allocated=0;
    library_init(&fresh);
    if(error&&cap)error[0]='\0';
    if(!file){
        if(error&&cap)snprintf(error,cap,"Cannot open %s; run from the project folder",path);
        return 0;
    }
    if(!fgets(line,sizeof(line),file))goto fail;
    normalize(line);
    if(strcmp(line,"id,title,author,subject,keywords,description,copies,hours,value")!=0)goto fail;
    while(fgets(line,sizeof(line),file)){
        char f[9][2050];
        Book b;
        char*part;
        int capacity;
        ++row;
        if(line[0]=='\n'||line[0]=='\r')continue;
        if(strlen(line)>8192||!csv_fields(line,f)||fresh.count>=MAX_BOOKS)goto fail;
        memset(&b,0,sizeof(b));
        if(!parse_int(f[0],&b.id)||!parse_int(f[6],&b.copies)||!parse_int(f[7],&b.hours)||!parse_int(f[8],&b.value))goto fail;
        for(int i=1;i<=5;++i)normalize(f[i]);
        if(b.id<1||b.copies<0||b.copies>100||b.hours<1||b.hours>10000||b.value<0||b.value>1000000||!f[1][0]||!f[2][0]||!f[3][0])goto fail;
        if(!copy_text(b.title,sizeof(b.title),f[1])||!copy_text(b.author,sizeof(b.author),f[2])||!copy_text(b.subject,sizeof(b.subject),f[3])||!copy_text(b.description,sizeof(b.description),f[5]))goto fail;
        part=strtok(f[4],";");
        while(part){
            normalize(part);
            if(*part){
                int duplicate=0;
                for(int k=0;k<b.keyword_count;++k)if(strcmp(b.keywords[k],part)==0)duplicate=1;
                if(!duplicate){
                    if(b.keyword_count==MAX_KEYWORDS||!copy_text(b.keywords[b.keyword_count],65,part))goto fail;
                    ++b.keyword_count;
                }
            }
            part=strtok(NULL,";");
        }
        if(!b.keyword_count)goto fail;
        if(fresh.count==allocated){
            capacity=allocated?allocated*2:16;
            if(capacity>MAX_BOOKS)capacity=MAX_BOOKS;
            Book *next=realloc(fresh.books,(size_t)capacity*sizeof(Book));
            if(!next)goto fail;
            fresh.books=next;
            allocated=capacity;
        }
        fresh.books[fresh.count++]=b;
    }
    if(ferror(file)||!fresh.count)goto fail;
    Book *tmp=malloc((size_t)fresh.count*sizeof(Book));
    if(!tmp)goto fail;
    merge_books(fresh.books,tmp,0,fresh.count);
    free(tmp);
    for(int i=1;i<fresh.count;++i)if(fresh.books[i-1].id==fresh.books[i].id)goto fail;
    if(!build_index(&fresh))goto fail;
    fresh.reader_count=1;
    fresh.readers[0].id=101;
    strcpy(fresh.readers[0].name,"Kushagra");
    fresh.readers[0].history[0]=fresh.books[0].id;
    fresh.readers[0].history_count=1;
    if(fresh.count>1){
        fresh.reader_count=2;
        fresh.readers[1].id=102;
        strcpy(fresh.readers[1].name,"Reader B");
        fresh.readers[1].history[0]=fresh.books[1].id;
        fresh.readers[1].history_count=1;
        fresh.citation_count=1;
        fresh.citations[0]=(Citation){
            fresh.books[1].id,fresh.books[0].id
        };
    }
    library_free(lib);
    *lib=fresh;
    ok=1;
    fail:     fclose(file);
    if(!ok){
        library_free(&fresh);
        if(error&&cap)snprintf(error,cap,"Invalid CSV or insufficient memory at/after row %d (check header, limits and duplicate IDs)",row);
    }
    return ok;
}
static int book_rec(const Book*a,int lo,int hi,int id){
    if(lo>hi)return -1;
    int mid=lo+(hi-lo)/2;
    if(a[mid].id==id)return mid;
    if(a[mid].id<id)return book_rec(a,mid+1,hi,id);
    return book_rec(a,lo,mid-1,id);
}
const Book*find_book(const Library*lib,int id,int recursive){
    int lo=0,hi=lib->count-1,at=-1;
    if(recursive)at=book_rec(lib->books,lo,hi,id);
    else while(lo<=hi){
        int mid=lo+(hi-lo)/2;
        if(lib->books[mid].id==id){
            at=mid;
            break;
        }
        if(lib->books[mid].id<id)lo=mid+1;
        else hi=mid-1;
    }
    return at<0?NULL:&lib->books[at];
}
int keyword_lookup(const Library*lib,const char*raw,int*ids,int cap){
    char word[129];
    int lo=0,hi=lib->index_count,count=0;
    if(strlen(raw)>128||cap<0)return -1;
    strcpy(word,raw);
    normalize(word);
    /* lower_bound: first index entry whose word is not smaller than query. */     while(lo<hi){
        int mid=lo+(hi-lo)/2;
        if(strcmp(lib->index[mid].word,word)<0)lo=mid+1;
        else hi=mid;
    }
    while(lo<lib->index_count&&strcmp(lib->index[lo].word,word)==0){
        if(count>=cap)return -1;
        ids[count++]=lib->index[lo++].book_id;
    }
    return count;
}
int search_books(const Library*lib,const char*raw,Matcher engine,Hit*hits,int cap,Counters*s){
    char query[129];
    int count=0;
    errno=0;
    if(strlen(raw)>128||cap<lib->count)return -1;
    strcpy(query,raw);
    normalize(query);
    if(!query[0])return -1;
    for(int i=0;i<lib->count;++i){
        const Book*b=&lib->books[i];
        double score;
        /* LIVE EDIT: change 3, 2 and 1 to change the relevance weights. */         score=3.0*match_text(b->title,query,engine,NULL,0,s)+1.0*match_text(b->description,query,engine,NULL,0,s);
        for(int k=0;k<b->keyword_count;++k)score+=2.0*match_text(b->keywords[k],query,engine,NULL,0,s);
        if(errno==ENOMEM)return -1;
        if(score>0)hits[count++]=(Hit){
            b->id,score
        };
    }
    if(!rank_hits(hits,count,s))return -1;
    return count;
}
int recommend_books(const Library*lib,int id,Hit*hits,int cap){
    const Book*target=find_book(lib,id,0);
    int count=0;
    if(!target||cap<lib->count)return -1;
    for(int i=0;i<lib->count;++i){
        const Book*b=&lib->books[i];
        int shared=0,same_subject;
        if(b->id==id)continue;
        for(int j=0;j<target->keyword_count;++j)for(int k=0;k<b->keyword_count;++k)if(strcmp(target->keywords[j],b->keywords[k])==0)++shared;
        same_subject=strcmp(target->subject,b->subject)==0;
        if(!shared&&!same_subject)continue;
        double jaccard=(double)shared/(target->keyword_count+b->keyword_count-shared);
        int title_lcs=lcs(target->title,b->title,NULL,0,NULL);
        if(title_lcs<0)return -1;
        size_t longest=strlen(target->title)>strlen(b->title)?strlen(target->title):strlen(b->title);
        /* LIVE EDIT: .5 keywords, .3 subject, .2 title similarity. */         double score=.5*jaccard+.3*same_subject+.2*title_lcs/longest;
        hits[count++]=(Hit){
            b->id,score
        };
    }
    if(!rank_hits(hits,count,NULL))return -1;
    return count>5?5:count;
}
static void csv_quote(FILE*f,const char*s){
    fputc('"',f);
    for(;*s;++s){
        if(*s=='"')fputc('"',f);
        fputc(*s,f);
    }
    fputc('"',f);
}
int export_reports(const Library*lib,const char*folder){
    char path[512];
    FILE*cat=NULL,*idx=NULL,*rec=NULL,*rel=NULL;
    Hit*hits=NULL;
    int ok=0;
    if(strlen(folder)>400||!make_directory(folder))return 0;
    snprintf(path,sizeof(path),"%s/catalog.csv",folder);
    cat=fopen(path,"w");
    snprintf(path,sizeof(path),"%s/index.csv",folder);
    idx=fopen(path,"w");
    snprintf(path,sizeof(path),"%s/recommendations.csv",folder);
    rec=fopen(path,"w");
    snprintf(path,sizeof(path),"%s/relationships.csv",folder);
    rel=fopen(path,"w");
    if(!cat||!idx||!rec||!rel)goto done;
    fprintf(cat,"id,title,author,subject,keywords,description,copies,hours,value\n");
    for(int i=0;i<lib->count;++i){
        const Book*b=&lib->books[i];
        char keys[2112]="";
        for(int k=0;k<b->keyword_count;++k){
            if(k)strcat(keys,";");
            strcat(keys,b->keywords[k]);
        }
        fprintf(cat,"%d,",b->id);
        csv_quote(cat,b->title);
        fputc(',',cat);
        csv_quote(cat,b->author);
        fputc(',',cat);
        csv_quote(cat,b->subject);
        fputc(',',cat);
        csv_quote(cat,keys);
        fputc(',',cat);
        csv_quote(cat,b->description);
        fprintf(cat,",%d,%d,%d\n",b->copies,b->hours,b->value);
    }
    fprintf(idx,"keyword,book_id\n");
    for(int i=0;i<lib->index_count;++i){
        csv_quote(idx,lib->index[i].word);
        fprintf(idx,",%d\n",lib->index[i].book_id);
    }
    fprintf(rec,"source_id,recommended_id,score\n");
    hits=malloc((size_t)lib->count*sizeof(*hits));
    if(!hits)goto done;
    int n=recommend_books(lib,lib->books[0].id,hits,lib->count);
    if(n<0)goto done;
    for(int i=0;i<n;++i)fprintf(rec,"%d,%d,%.12g\n",lib->books[0].id,hits[i].id,hits[i].score);
    fprintf(rel,"type,from,to\n");
    for(int i=0;i<lib->reader_count;++i)for(int j=0;j<lib->readers[i].history_count;++j)fprintf(rel,"read,%d,%d\n",lib->readers[i].id,lib->readers[i].history[j]);
    for(int i=0;i<lib->citation_count;++i)fprintf(rel,"cites,%d,%d\n",lib->citations[i].from,lib->citations[i].to);
    ok=!ferror(cat)&&!ferror(idx)&&!ferror(rec)&&!ferror(rel);
    done:     free(hits);
    if(cat&&fclose(cat))ok=0;
    if(idx&&fclose(idx))ok=0;
    if(rec&&fclose(rec))ok=0;
    if(rel&&fclose(rel))ok=0;
    return ok;
}
