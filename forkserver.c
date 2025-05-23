#include "config.h"
#include <sanitizer/coverage_interface.h>

int shmid;
char initial_bitmap[SHM_SIZE];
char* shared_memory = initial_bitmap;
int total_guards = 0; 
__attribute__((no_sanitize("undefined")))

void init_shared(){
    printf("ENV NAME: %s\n",SHM_ENV_VAR);
    char* shm_str = getenv(SHM_ENV_VAR);
    printf("child shm_str: %s\n", shm_str);
    if (shm_str) {
        shmid = atoi(shm_str);
        printf("shmid: %d\n", shmid);
        shared_memory = shmat(shmid, NULL, 0);

        if (shared_memory == (void*)-1) {
            perror("shmat() failed");
            exit(1);
        }
    }
    else{
        perror("shm_str failed");
        exit(0);
    }
    
}

void init_forkserver(){
    int do_fork;

    if (write(FORKSRV_FD+1, &total_guards, sizeof(int)) != sizeof(int)) {
        return;
    }

    while(1){
        pid_t target_pid;
        int status;
        if (read(FORKSRV_FD, &do_fork, sizeof(int)) != sizeof(int)) {
            exit(1);
        }
        target_pid = fork();
        if(target_pid<0){
            perror("forkserver failed");
            exit(1);
        }
        if(target_pid == 0){
            //child - target -> 해당 코드는 test 바이너리에서 실행되는 것이고 main 전에 실행되도록 했으므로 return 시에는 main이 실행 될 것이다.
            close(FORKSRV_FD);
            close(FORKSRV_FD+1);

            return ;
        }
        else{
            //parent - fuzzer와 통신 담당
            //fuzzer에게 target pid 전달
            if(write(FORKSRV_FD+1, &target_pid, sizeof(pid_t)) != sizeof(pid_t)) {
                exit(1);
            }
            // target 끝날때까지 기다리기
            waitpid(target_pid, &status, 0);
            
            // 끝난거 알려주기
            if(write(FORKSRV_FD+1, &status, sizeof(int)) != sizeof(int)) {
                exit(1);
            }
            

        }
    }
}

__attribute__((constructor)) void init_target() {
    init_shared();
    init_forkserver();
}

void __sanitizer_cov_trace_pc_guard_init(uint32_t *start,
                                                    uint32_t *stop) {
    static uint64_t N;  // Counter for the guards.
    if (start == stop || *start) return;  // Initialize only once.
    printf("INIT: %p %p\n", start, stop);
    for (uint32_t *x = start; x < stop; x++)
    *x = ++N;  // Guards should start from 1.
    total_guards = N;

}

void __sanitizer_cov_trace_pc_guard(uint32_t *guard) {

    if (!*guard) return;
    shared_memory[*guard]++;

}