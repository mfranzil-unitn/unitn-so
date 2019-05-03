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
#include <time.h>
#include <unistd.h>

// Frigo = 2

char* pipe_fd = NULL;
char log_buf[512];
int pid, name, delay, perc, temp;

int status = 0;  // interruttore accensione
time_t start;

void sighandle_usr1(int sig) {
    int fd = open(pipe_fd, O_WRONLY);
    time_t time_on;
    char tmp[1024];

    if (status) {
        time_on = (time(NULL) - start);
    } else {
        time_on = 0;
    }

    sprintf(tmp, "2|%i|%i|%i|%i|%i|%i|%i|%s",
            status, (int)time_on, pid, name, delay, perc, temp, log_buf);

    write(fd, tmp, 1024);

    log_buf[0] = '\0';

    close(fd);
}

void sighandle_usr2(int sig) {
    // Al ricevimento del segnale, il frigo apre la pipe in lettura e ottiene cosa deve fare.
    // 1|... -> chiudi/apri frigo
    // 2|TEMP -> setta temperatura del frigo

 /*   int fd = open(pipe_fd, O_RDONLY);
    char tmp[1024];
    read(fd, tmp, 1024);
*/     

    if (!status) {
        status = 1;
        start = time(NULL);
    } else {
        status = 0;
        start = 0;
    }
}

int main(int argc, char* argv[]) {
    // argv = [./fridge, indice, /tmp/indice];
    pipe_fd = argv[2];
    pid = getpid();
    name = atoi(argv[1]);

    delay = 15;
    perc = 50;
    temp = 3;

    log_buf[0] = '\0'; // buffer di log vuoto

    signal(SIGUSR1, sighandle_usr1);
    signal(SIGUSR2, sighandle_usr2);

    while (1) {
        if (status == 1 && start < time(NULL) - delay) {
            status = 0;
            sprintf(log_buf, "Il frigorifero %d si è chiuso automaticamente dopo %d secondi",
                    name, delay);
            // Problema di estetica
        }
        sleep(1);
    }

    return 0;
}
