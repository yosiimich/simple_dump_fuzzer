#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/time.h>
#include <sys/stat.h>

#define SHM_ENV_VAR         "__SHM_ID"
#define FORK_WAIT_MULT      10
#define EXEC_TIMEOUT        1000
#define SHM_SIZE            65536
// FORKSRV_FD -> read
// FORKSRV_FD + 1-> write
int FORKSRV_FD = 138;
#endif // CONFIG_H