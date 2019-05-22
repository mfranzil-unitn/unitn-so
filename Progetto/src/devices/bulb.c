#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../util.h"

// BULB = 1

int shellpid;
int fd;                // file descriptor della pipe verso il padre
char* pipe_fd = NULL;  // nome della pipe

int pid, __index;  // variabili di stato

int status = 0;  // interruttore accensione
time_t start;

void sighandle_sigterm(int signal) {
    /*if ((int)getppid() != shellpid) {
        int ppid = (int)getppid();
        kill(ppid, SIGUSR2);
        char pipe_str[MAX_BUF_SIZE];
        get_pipe_name(ppid, pipe_str);  // Nome della pipe
        int fd = open(pipe_str, O_RDWR);
        char tmp[MAX_BUF_SIZE];
        sprintf(tmp, "2|%d", (int)getpid());
        write(fd, tmp, sizeof(tmp));
    }*/
    exit(0);
}

void sighandle_usr1(int sig) {
    time_t time_on;
    char buffer[MAX_BUF_SIZE];

    if (status) {
        time_on = (time(NULL) - start);
    } else {
        time_on = 0;
    }

    sprintf(buffer, "1|%i|%i|%i|%i",
            pid, __index, status, (int)time_on);

    write(fd, buffer, MAX_BUF_SIZE);
}

void sighandle_usr2(int sig) {

    /* Al ricevimento del segnale, la finestra apre la pipe in lettura e ottiene cosa deve fare.
    0|... -> accendi/spegni lampadina
    1|... -> restituisci PID*/
    char tmp[MAX_BUF_SIZE];
    char** vars;
    
    vars = split_fixed(tmp, 2);
    lprintf("Entering user2\n");
    read(fd, tmp, MAX_BUF_SIZE);
    char** vars = split_fixed(tmp, 2);

    if (atoi(vars[0]) == 0) {
        if (!status) {
            status = 1;
            start = time(NULL);
        } else {
            status = 0;
            start = 0;
        }
    }
}

int main(int argc, char* argv[]) {
    // argv = [./bulb, indice, /tmp/indice];
    pipe_fd = argv[2];
    pid = getpid();
    __index = atoi(argv[1]);
    fd = open(pipe_fd, O_RDWR);

    shellpid = get_shell_pid();
    signal(SIGTERM, sighandle_sigterm);
    signal(SIGUSR1, sighandle_usr1);
    signal(SIGUSR2, sighandle_usr2);
    while (1) {
        sleep(10);
    }

    return 0;
}