#include "library.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif
static int read_line(const char *prompt,char *out,size_t capacity) {
    printf("%s",prompt);
    if(!fgets(out,(int)capacity,stdin))return 0;
    if(!strchr(out,'\n')&&!feof(stdin)){
        int ch;
        while((ch=getchar())!='\n'&&ch!=EOF){
        }
        out[0]='\0';
        puts("Input too long.");
        return 1;
    }
    out[strcspn(out,"\r\n")]='\0';
    return 1;
}
static int read_number(const char*prompt,int*out){
    char line[64],*end;
    long value;
    if(!read_line(prompt,line,sizeof(line)))return 0;
    errno=0;
    value=strtol(line,&end,10);
    if(errno||end==line||*end||value<0||value>1000000000){
        puts("Enter a nonnegative whole number.");
        *out=-1;
        return 1;
    }
    *out=(int)value;
    return 1;
}
static void print_hits(const Library*lib,const Hit*hits,int n){
    if(n<0){
        puts("Invalid input or insufficient memory.");
        return;
    }
    if(!n)puts("No results.");
    for(int i=0;i<n;++i){
        const Book*b=find_book(lib,hits[i].id,0);
        printf("ID %d | %s | score %.3f\n",hits[i].id,b->title,hits[i].score);
    }
}
static void locate_data(const char *program){
    FILE*f=fopen("data/books.csv","r");
    if(f){
        fclose(f);
        return;
    }
    char path[1024];
    if(strlen(program)>=sizeof(path))return;
    strcpy(path,program);
    char *slash=strrchr(path,'/'),*back=strrchr(path,'\\');
    if(back&&(!slash||back>slash))slash=back;
    if(!slash)return;
    *slash='\0';
#ifdef _WIN32
    if(_chdir(path)!=0)return;
#else
    if(chdir(path)!=0)return;
#endif
    f=fopen("data/books.csv","r");
    if(f){
        fclose(f);
        return;
    }
#ifdef _WIN32
    (void)_chdir("..");
#else
    (void)chdir("..");
#endif
}
int main(int argc,char**argv){
    Library lib;
    char error[256];
    int status=0;
    library_init(&lib);
    locate_data(argv[0]);
    if(argc>1&&strcmp(argv[1],"--test")==0)return run_tests(stdout)?0:1;
    if(argc>1&&strcmp(argv[1],"--lab")==0)return run_lab(stdout)?0:1;
    if(argc>1&&strcmp(argv[1],"--bench")==0)return run_benchmarks(stdout)?0:1;
    if(!library_load(&lib,"data/books.csv",error,sizeof(error))){
        fprintf(stderr,"%s\n",error);
        return 1;
    }
    if(argc>1){
        if(strcmp(argv[1],"--demo")==0){
            if(!make_directory("results")){
                library_free(&lib);
                return 1;
            }
            FILE*f=fopen("results/scenarios.txt","w");
            status=f&&run_scenarios(&lib,f)&&run_scenarios(&lib,stdout)&&run_benchmarks(stdout)&&export_reports(&lib,"results");
            if(f&&fclose(f))status=0;
            library_free(&lib);
            return status?0:1;
        }
        fprintf(stderr,"Use --test, --lab, --bench or --demo.\n");
        library_free(&lib);
        return 1;
    }
    printf("LibraDAA | Simple Digital Library | Pure C\nLoaded %d books.\n",lib.count);
    for(;;){
        int choice;
        puts("\n1 Search books\n2 Recommend books\n3 Required scenarios + benchmarks\n4 Algorithm lab\n5 Load / export CSV\n0 Exit");
        if(!read_number("Choice: ",&choice)||choice==0)break;
        if(choice==1){
            int kind;
            if(!read_number("Search type: 1 substring, 2 exact ID, 3 keyword index: ",&kind))break;
            if(kind==2){
                int id;
                if(!read_number("Book ID: ",&id))break;
                const Book*a=find_book(&lib,id,0),*b=find_book(&lib,id,1);
                printf("Iterative/recursive agreement: %s\n",a==b?"yes":"no");
                puts(a?a->title:"Not found");
            }
            else if(kind==3){
                char word[129];
                if(!read_line("Keyword: ",word,sizeof(word)))break;
                int*ids=malloc((size_t)lib.count*sizeof(int));
                if(!ids){
                    puts("Not enough memory.");
                    continue;
                }
                int n=keyword_lookup(&lib,word,ids,lib.count);
                for(int i=0;i<n;++i)printf("%d | %s\n",ids[i],find_book(&lib,ids[i],0)->title);
                if(n==0)puts("No results.");
                free(ids);
            }
            else if(kind==1){
                char query[129];
                int engine;
                if(!read_line("Word / phrase: ",query,sizeof(query)))break;
                if(!read_number("Engine 0 naive, 1 KMP, 2 Rabin-Karp, 3 automaton: ",&engine))break;
                if(engine<0||engine>3){
                    puts("Choose engine 0..3.");
                    continue;
                }
                Hit*hits=malloc((size_t)lib.count*sizeof(Hit));
                Counters stats;
                counters_reset(&stats);
                if(!hits){
                    puts("Not enough memory.");
                    continue;
                }
                int n=search_books(&lib,query,(Matcher)engine,hits,lib.count,&stats);
                print_hits(&lib,hits,n);
                printf("Operations: %lld | modeled auxiliary bytes: %lld\n",stats.comparisons,stats.aux_bytes);
                puts("Score = 3*title matches + 2*keyword matches + description matches.");
                if(n>0){
                    size_t pos[256];
                    normalize(query);
                    const Book*b=find_book(&lib,hits[0].id,0);
                    size_t total=match_text(b->title,query,(Matcher)engine,pos,256,NULL);
                    printf("First result title positions (zero-based):");
                    for(size_t i=0;i<total&&i<256;++i)printf(" %zu",pos[i]);
                    putchar('\n');
                }
                free(hits);
            }
            else puts("Choose search type 1..3.");
        }
        else if(choice==2){
            int id;
            if(!read_number("Book ID: ",&id))break;
            Hit*hits=malloc((size_t)lib.count*sizeof(Hit));
            if(!hits){
                puts("Not enough memory.");
                continue;
            }
            print_hits(&lib,hits,recommend_books(&lib,id,hits,lib.count));
            free(hits);
            puts("Recommendation = .5*keyword overlap + .3*same subject + .2*normalized title LCS.");
        }
        else if(choice==3){
            if(!run_scenarios(&lib,stdout)||!run_benchmarks(stdout))puts("Scenario or benchmark failed.");
        }
        else if(choice==4){
            if(!run_lab(stdout))puts("Algorithm lab failed.");
        }
        else if(choice==5){
            int action;
            if(!read_number("1 Load CSV, 2 export reports: ",&action))break;
            if(action==1){
                char path[512];
                if(!read_line("CSV path: ",path,sizeof(path)))break;
                if(!library_load(&lib,path,error,sizeof(error)))puts(error);
                else printf("Loaded %d books.\n",lib.count);
            }
            else if(action==2)puts(export_reports(&lib,"results")?"Saved CSV reports in results/.":"Export failed.");
            else puts("Choose 1 or 2.");
        }
        else puts("Choose 0..5.");
    }
    library_free(&lib);
    return 0;
}
