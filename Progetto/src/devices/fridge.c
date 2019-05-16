#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../util.h"

// Frigo = 2

int fd;                          // file descriptor della pipe verso il padre
char* pipe_fd = NULL;            // nome della pipe
char log_buf[MAX_BUF_SIZE / 4];  // buffer della pipe // SE CI SONO PROBLEMI; GUARDA QUI

int pid, __index, delay, perc, temp;  // variabili di stato

int status = 0;  // interruttore accensione
time_t start;

void sighandle_usr1(int sig) {
    time_t time_on;
    char tmp[MAX_BUF_SIZE];

    if (status) {
        time_on = (time(NULL) - start);
    } else {
        time_on = 0;
    }

    sprintf(tmp, "2|%i|%i|%i|%i|%i|%i|%i|%s",
            pid, __index, status, (int)time_on, delay, perc, temp, log_buf);

    write(fd, tmp, MAX_BUF_SIZE);

    // Resetto il contenuto del buffer
    sprintf(log_buf, "-");
}

void sighandle_usr2(int sig) {
    // Al ricevimento del segnale, il frigo apre la pipe in lettura e ottiene cosa deve fare.
    // 0|... -> chiudi/apri frigo
    // 1|TEMP -> setta temperatura del frigo
    // 2|DELAY -> setta delay di chiusura
    // 3|PERCENT -> setta contenuto
    char tmp[MAX_BUF_SIZE];

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
    } else if (atoi(vars[0]) == 1) {
        temp = atoi(vars[1]);
    } else if (atoi(vars[0]) == 2) {
        delay = atoi(vars[1]);
    } else if (atoi(vars[0]) == 3) {
        perc = atoi(vars[1]);
    }
}

int main(int argc, char* argv[]) {
    // argv = [./fridge, indice, /tmp/indice];
    pipe_fd = argv[2];
    pid = getpid();
    __index = atoi(argv[1]);

    delay = 15;
    perc = 50;
    temp = 5;

    sprintf(log_buf, "-");

    fd = open(pipe_fd, O_RDWR);

    signal(SIGUSR1, sighandle_usr1);
    signal(SIGUSR2, sighandle_usr2);

    while (1) {
        if (status == 1 && start < time(NULL) - delay) {
            status = 0;
            sprintf(log_buf,
                    "Il frigorifero %d si è chiuso automaticamente dopo %d secondi",
                    __index, delay);
        }
        sleep(1);
    }

    return 0;
}
