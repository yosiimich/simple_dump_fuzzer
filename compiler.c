#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char** compile_parm;
int c_p_cnt=1;

int main(int argc, char* argv[]){
    if(argc<2){
        perror("DEFAULT USAGE: ./complie <target>.c -o <binary_name>");
        return 1;
    }

    compile_parm = (char**)malloc((argc+16)*sizeof(char*));
    memset(compile_parm, 0, (argc + 16) * sizeof(char*));

    compile_parm[0] = "clang";
    //compile_parm[c_p_cnt++] = "-fsanitize-coverage=trace-pc-guard";
    compile_parm[c_p_cnt++] = "-fsanitize=address";
    //compile_parm[c_p_cnt++] = "-O0";
    //compile_parm[c_p_cnt++] = "-fno-sanitize=all";
    
    for(int i = 1; i<argc; i++){
        compile_parm[c_p_cnt++] = argv[i];
    }

    compile_parm[c_p_cnt++] = "forkserver.o";
    compile_parm[c_p_cnt] = NULL;

    execvp(compile_parm[0],compile_parm);

    return 0;
}