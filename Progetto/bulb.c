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
#include <time.h>

// BULB = 1

#define SIGON 901

char* pipe_fd = NULL;
int pid, name;

int status = 0; // interruttore accensione
time_t start;


void sighandle_usr1(int sig) {
    int fd = open(pipe_fd, O_WRONLY);
    time_t time_on;
    char buffer[1024];

    if (status) {
        time_on = (time(NULL) - start);
    } else {
        time_on = 0;
    }

    sprintf(buffer, "1|%i|%i|%i|%i",
        status, (int) time_on, pid, name);
    
    write(fd, buffer, 1024);
    close(fd);
}

void sighandle_usr2(int sig) {
    if (!status) {
    status = 1;
    start = time(NULL);
    } else {
    status = 0;
    start = 0;
    }
}

int main(int argc, char* argv[]) {
    // argv = [./bulb, indice, /tmp/indice];
    pipe_fd = argv[2];
    pid = getpid();
    name = atoi(argv[1]);

    // Converto il pid in stringa (che palle...)
   // pid_str = malloc(4 * sizeof(char));
   // sprintf(pid_str, "%d", pid);

    signal(SIGUSR1, sighandle_usr1);
    signal(SIGUSR2, sighandle_usr2);
    while(1);

    return 0;
}