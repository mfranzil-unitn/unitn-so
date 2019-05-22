#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../util.h"

/* BULB = 1 */

int shellpid;
int fd;               /* file descriptor della pipe verso il padre */
char* pipe_fd = NULL; /* nome della pipe */

int pid, __index; /* variabili di stato */

int status = 0; /* interruttore accensione */
time_t start;

struct tm tm_start;
struct tm tm_end;
struct tm tm_current;

void sighandle_sigterm(int signal) {
    /*if ((int)getppid() != shellpid) {
        int ppid = (int)getppid();
        kill(ppid, SIGUSR2);
        char pipe_str[MAX_BUF_SIZE];
        get_pipe_name(ppid, pipe_str);   Nome della pipe 
        int fd = open(pipe_str, O_RDWR);
        char tmp[MAX_BUF_SIZE];
        sprintf(tmp, "2|%d", (int)getpid());
        write(fd, tmp, sizeof(tmp));
    }*/
    exit(0);
}

void sighandle_usr1(int sig) {
    char buffer[MAX_BUF_SIZE];

    sprintf(buffer, "5|%i|%i|%i|%i|%i|%i",
            pid, __index, status,
            tm_start.tm_hour, tm_start.tm_min,
            tm_end.tm_hour, tm_end.tm_min);
    write(fd, buffer, MAX_BUF_SIZE);
}

void sighandle_usr2(int sig) {
    // Al ricevimento del segnale, la finestra apre la pipe in lettura e ottiene cosa deve fare.
    // 0|ORA|MINUTI|ORAFINE|MINUTIFINE -> imposta timer
    char tmp[MAX_BUF_SIZE];
    char** vars;

    int mode = tmp[0] - '0';

    lprintf("Entering user2\n");
    read(fd, tmp, MAX_BUF_SIZE);

    if (mode == 0) {
        vars = split_fixed(tmp, 5);

        tm_start = *localtime(&(time_t){time(NULL)});
        tm_end = *localtime(&(time_t){time(NULL)});

        tm_start.tm_hour = atoi(vars[1]);
        tm_start.tm_min = atoi(vars[2]);
        tm_end.tm_hour = atoi(vars[3]);
        tm_end.tm_min = atoi(vars[4]);
    }
}

int check_time() {
    tm_current = *localtime(&(time_t){time(NULL)});
    if (tm_current.tm_hour == tm_start.tm_hour && tm_current.tm_min == tm_start.tm_min) {
        /* Accendo il dispositivo sotto... */
        status = 1;
    } else if (tm_current.tm_hour == tm_end.tm_hour && tm_current.tm_min == tm_end.tm_min) {
        status = 0;
    } else {
    }
}

int main(int argc, char* argv[]) {
    /* argv = [./timer, indice, /tmp/pid]; */
    pipe_fd = argv[2];
    pid = getpid();
    __index = atoi(argv[1]);
    fd = open(pipe_fd, O_RDWR);

    shellpid = get_shell_pid();
    signal(SIGTERM, sighandle_sigterm);
    signal(SIGUSR1, sighandle_usr1);
    signal(SIGUSR2, sighandle_usr2);

    while (1) {
        check_time();
        sleep(10);
    }

    return 0;
}