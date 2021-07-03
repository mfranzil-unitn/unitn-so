// sys.c | Example of usage: ./sys “ls -l”
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
    int r;
    char cmd[100];
    printf("argc=%d\n", argc);
    if (argc > 1) {
        strcpy(cmd, argv[1]);
    } else {
        strcpy(cmd, "");
    };
    printf("cmd=%s\n", cmd);
    r = system(cmd);
    printf("R,errno=%d,%d\n", r, errno);
    // in case of "logical" error running cmd
    // R is the error code, while errno should be "0"
    // as the syscall has beenrun
    return r;
}