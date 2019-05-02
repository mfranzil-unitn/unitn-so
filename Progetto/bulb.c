#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

char* pipe_pos = NULL;
int pid;
char* pid_str;

void sighandle_usr1(int sig) {
    int fd = open(pipe_pos, O_WRONLY);
    write(fd, pid_str, strlen(pid_str) + 1);
    close(fd);
}

int main(int argc, char* argv[]) {
    // argv = [./bulb, indice, /tmp/indice];
    pipe_pos = argv[2];
    int pid = getpid();

    // Converto il pid in stringa (che palle...)
    pid_str = malloc(4 * sizeof(char));
    sprintf(pid_str, "%d", pid);

    signal(SIGUSR1, sighandle_usr1);

    pause();

    return 0;
}