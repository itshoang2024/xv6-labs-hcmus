#include "kernel/types.h"
#include "user.h"

void sieve(int pipeLeft[2]) __attribute__((noreturn));

void sieve(int pipeLeft[2]) {
    close(pipeLeft[1]);
    int prime, n;
    
    if(read(pipeLeft[0], &prime, sizeof(int)) == 0) {
        exit(0);
    }
    fprintf(1, "prime %d\n", prime);
    
    int pipeRight[2];
    pipe(pipeRight);
    int pid = fork();
    if(pid) {
        close(pipeRight[0]);
        while(read(pipeLeft[0], &n, sizeof(int)) > 0) {
            if(n % prime != 0) {
                write(pipeRight[1], &n, sizeof(int));
            }
        }
        close(pipeRight[1]);
        close(pipeLeft[0]);
        wait(0);
        exit(0);
    } else {
        close(pipeLeft[0]);
        close(pipeRight[1]);
        sieve(pipeRight);
    }
}

int main(int argc, char* argv[]) {
    int pipeMain[2];
    pipe(pipeMain);

    int pid = fork();
    if(!pid) {
        close(pipeMain[1]);
        sieve(pipeMain);
    } else {
        close(pipeMain[0]);
        for(int i = 2; i <= 280; i++) {
            write(pipeMain[1], &i, sizeof(int));
        }
        close(pipeMain[1]);
        wait(0); 
    }
    
    exit(0);
}