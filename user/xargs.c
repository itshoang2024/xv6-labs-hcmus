#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

#define MAXLINE 512 

int getcmd(char *buf, int nbuf) {
    memset(buf, 0, nbuf);
    int n = read(0, buf, nbuf - 1);
    if (n <= 0) return -1;
    buf[n] = 0;
    return 0;
}

char whitespace[] = " \t\r\n\v";

int gettoken(char **ps, char *es, char **q, char **eq) {
    char *s = *ps;

    while (s < es && strchr(whitespace, *s)) s++;
    if (q) *q = s;

    int ret = *s;
    if (*s) {  
        ret = 'a';
        while (s < es && !strchr(whitespace, *s)) s++;
    }

    if (eq) *eq = s;
    if (s < es) *s++ = 0;

    while (s < es && strchr(whitespace, *s)) s++;
    *ps = s;
    return ret;
}

int main(int argc, char *argv[]) {
    char *xargs[MAXARG]; 
    for (int i = 1; i < argc; i++) {
        xargs[i - 1] = argv[i];  
    }

    static char buf[MAXLINE];
    char *q, *eq;

    while (getcmd(buf, sizeof(buf)) >= 0) {
        int j = argc - 1;  
        char *s = buf;
        char *es = s + strlen(s);

        while (gettoken(&s, es, &q, &eq) != 0) {
            *eq = 0;  
            xargs[j] = q;
            xargs[j + 1] = 0;  

            int pid = fork();
            if (pid == 0) {  
                exec(xargs[0], xargs);
                fprintf(2, "xargs: exec failed for %s\n", xargs[0]);
                exit(1);
            } else {  
                wait(0);
            }
        }
    }

    exit(0);
}
