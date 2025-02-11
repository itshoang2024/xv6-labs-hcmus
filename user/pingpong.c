#include "kernel/types.h"
#include "user.h"

int main(int argc, char* argv[]) {
    int pipePing[2], pipePong[2];
    if(pipe(pipePing) == -1 || pipe(pipePong) == -1) {
        fprintf(2, "Error: can't create pipe\n");
        exit(1);
    }


    char buffer = 'c';
    int pid = fork();
    if(pid < 0) {
        fprintf(2, "Error: can't fork\n");
        exit(1);
    }

    if(pid == 0) { // child process
        close(pipePing[1]);
        close(pipePong[0]);

        if(read(pipePing[0], &buffer, 1) == -1) {
            fprintf(2, "Error: child can't read\n");
            exit(1);
        }
        fprintf(1, "%d: received ping\n", getpid());
        close(pipePing[0]);

        if(write(pipePong[1], &buffer, 1) == -1) {
            fprintf(2, "Error: child can't write\n");
            exit(1);
        }
        close(pipePong[1]);
        exit(0);
    }

    else { // parent process
        close(pipePing[0]);
        close(pipePong[1]);
        
        if(write(pipePing[1], &buffer, 1) == -1) {
            fprintf(2, "Error: parent can't write\n");
            exit(1);
        }
        close(pipePing[1]);

        if(read(pipePong[0], &buffer, 1) == -1) {
            fprintf(2, "Error: parent can't read\n");
            exit(1);
        }
        fprintf(1, "%d: received pong\n", getpid());
        close(pipePong[0]);
        wait(0);
        exit(0);
    }
}