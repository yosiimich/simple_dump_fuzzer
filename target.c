#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024

void crash(){
    ((void(*)())0)();
}

int main(int argc, char *argv[]) {
    
    if (argc < 2) {
        perror("Usage: ./test <filename>");
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        return 1;
    }

    char buffer[BUFFER_SIZE];


    printf("Reading from file: %s\n", argv[1]);
    memset(buffer,0,sizeof(buffer));
    read(fd, buffer, sizeof(buffer));

    if(buffer[0]==0x7f){
        if(buffer[1]==0x45){
            if(buffer[2]==0x4c){
                if(buffer[3]==0x46){
                    crash();
                }
            }
        }
    }
    
    close(fd);
    
    return 0;
}
