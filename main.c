#include "config.h"
#define MAX_SIZE 1024
#define DIR_PATH "./mutation_files"
#define FILE_TEMPLATE DIR_PATH "/mutation_file_%09u"
#define SYMLINK_PATH "./testfile"
#define CRASH_DIR_PATH "./crashes"

int fsrv_read;
int fsrv_write;
char *shared_memory;
pid_t child_pid;
pid_t forkserver_pid;
int mutation_file;
size_t length;
unsigned int file_count = 0;
unsigned char buffer[MAX_SIZE];
char filename[64];
int i = 0;
unsigned int total_guard=0;
int shmid=0;
clock_t start;
clock_t end;

void randomize_inputfile() {

    length = (rand() % MAX_SIZE) + 1;
    for (size_t i = 0; i < length; i++) {
        buffer[i] = rand() % 256;  // 0~255 범위의 랜덤 바이트
    }
    snprintf(filename, sizeof(filename), FILE_TEMPLATE, file_count++);
    mutation_file = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (mutation_file == -1) {
        perror("open file failed");
        exit(1);
    }

    // 데이터 쓰기
    if (write(mutation_file, buffer, length) == -1) {
        perror("write file failed");
        close(mutation_file);
        exit(1);
    }
    close(mutation_file);
    unlink(SYMLINK_PATH);
    if (symlink(filename, SYMLINK_PATH) == -1) {
        perror("symlink failed");
        exit(1);
    }
}

void remove_shm(){
    shmctl(shmid, IPC_RMID, NULL);
    end = clock();

    double elapsed_time = (double)(end - start) / CLOCKS_PER_SEC;

    // 시간, 분, 초로 변환
    int hours = (int)(elapsed_time / 3600);         // 1시간 = 3600초
    int minutes = (int)((elapsed_time - hours * 3600) / 60);  // 1분 = 60초
    double seconds = elapsed_time - hours * 3600 - minutes * 60; // 나머지 초는 소수점까지

    // 초를 소수점 아래 두 자리까지 출력
    printf("running time: %d h %d m %.6f s\n", hours, minutes, seconds);
}

void init_shm() {
    // 공유 메모리 생성
    shmid = shmget(IPC_PRIVATE, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(1);
    }
    atexit(remove_shm);

    char shm_str[16];
    memset(shm_str, 0, sizeof(shm_str));
    sprintf(shm_str, "%d", shmid);
    printf("shmid: %s\n", shm_str);

    if(setenv(SHM_ENV_VAR, shm_str, 1) != 0){
        perror("parent setenv failed");
        exit(0);
    }
    // 공유 메모리 연결
    shared_memory = shmat(shmid, NULL, 0);
    if (shared_memory == (void *)-1) {
        perror("shmat failed");
        exit(1);
    }

}

void print_shm(){
    unsigned int cov = 0;
    for (int i = 0; i < SHM_SIZE; i++){
        cov +=shared_memory[i];
    }
    printf("coverage: %.02f\n", ((float)cov/total_guard)*100);
}


void init_forkserver(char* argv[]){

    int ctl_pipe[2];
    int st_pipe[2];
    

    if(pipe(ctl_pipe) || pipe(st_pipe)){
        perror("pipe failed");
        exit(1);
    }

    forkserver_pid = fork();

    if(forkserver_pid<0){
        perror("forkserver failed");
        exit(1);
    }
    else if(forkserver_pid == 0){
        puts("3");

        int dev_null_fd = open("/dev/null", O_RDWR);
        /*
            해당 방식으로 /dev/null로 STDOUT_FILENO, STDERR_FILENO를 조정하면 앞으로의 출력값과 오류 값은 /dev/null에 쓰여지게 된다.
        */
        dup2(dev_null_fd, 1);
        dup2(dev_null_fd, 2);

        /*
            ctl_pipe[0]은 read를 의미한다.
            이걸 FORKSRV_FD라는 것에 등록해 두면 fuzzer에서 ctl_pipe[1]에 write로 입력 시 forkserver에서는 FORKSRV_FD를 read를 통해 읽을 수 있게 되는 것이다.
            해당 pipe는 부모가 자식에거 fork하라는 명령을 내리는 pipe이므로 이것을 읽게되면 forkserver에서 target이 fork 된다.
        */
        dup2(ctl_pipe[0], FORKSRV_FD); // 부모가 자식 컨트롤
        dup2(st_pipe[1], FORKSRV_FD + 1); // 자식이 부모한테 상태 전달

        close(ctl_pipe[0]);
        close(st_pipe[1]);
        // forkserver 실행 -> ./test aaaa
        execvp(argv[0],argv);
    }

    fsrv_read = st_pipe[0]; // 포크서버 상태 읽어오기
    fsrv_write = ctl_pipe[1]; // 포크서버에 포크하라는거 알릴때 사용
    if (read(fsrv_read, &total_guard, sizeof(int)) != sizeof(int)) {
        exit(1);
    }
}

void run_target(){

    int do_fork;
    pid_t child_pid;
    int status;

    memset(shared_memory, 0, SHM_SIZE);

    // make fuzz target
    puts("fuzzing start");
    if (write(fsrv_write, &do_fork, sizeof(int)) != sizeof(int)) exit(1);

    // read child_pid
    if (read(fsrv_read, &child_pid, sizeof(pid_t)) != sizeof(pid_t)) exit(1);
    printf("pid fuzz target: %d\n", child_pid);

    //read exit status of fuzz target
    if (read(fsrv_read, &status, sizeof(int)) != sizeof(int)) exit(1);
    printf("exit fuzz target: %d\n", status);

    if(status==256){
        printf("exit: %d\n", WIFEXITED(status));
        printf("crash occured in %s\n", filename);
        //mv_crash_file();
        exit(1);
    }

    randomize_inputfile();
}

void signal_hanlder(){

    printf("kill all process\n");
    if(forkserver_pid>0){
        kill(forkserver_pid, SIGKILL);
    }
    if(child_pid>0){
        kill(child_pid, SIGKILL);
    }

    exit(0);

}

void setup_signal(){
    signal(SIGINT,signal_hanlder);
    signal(SIGHUP,signal_hanlder);
    signal(SIGTERM,signal_hanlder);
}
__attribute__((no_sanitize("undefined")))

int main(int argc, char* argv[]) {
    
    start = clock();

    if(argc<2){
        printf("USAGE: ./main <binary> <argv1> ...\n");
        return 1;
    }

    // 인자 파싱
    char *exec_args[argc];
    memset(exec_args,0,argc);
    for (int i = 1; i < argc; i++) {
        exec_args[i - 1] = argv[i];  // argv[1]부터 복사
    }
    exec_args[argc - 1] = NULL;

    setup_signal();

    mkdir(DIR_PATH, 0755);
    mkdir(CRASH_DIR_PATH, 0755);
    // fork-server 초기화
    init_shm();
    init_forkserver(exec_args);

    while(1){
        run_target();
        print_shm();
    }
    
    return 0;
}
