#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include "../util.h"

// Window = 3

/*
Suppongo che i due interruttori che comandano la finestra Interr_Aperto e
 Interr_Chiuso siano sempre opposti. Quando ne premo uno questo resta in uno
  stato OFF (non può più essere premuto) e l'altro cambia nello stato ON (può essere premuto).

Sia lo switch della centarlina sia lo switch "manuale" modificano lo stato degli interruttori

Con il comando switch permetto solo: switch <id> <Interr_Aperto/Chiuso> ON   	
*/

int fd;                // file descriptor della pipe verso il padre
char* pipe_fd = NULL;  // nome della pipe
//char log_buf[512];     // buffer della pipe

int pid, __index, delay;  // variabili di stato

int status = 0;  // interruttore apertura
time_t start;

void sighandle_usr1(int sig) {
    time_t time_on;
    char buffer[MAX_BUF_SIZE];

    if (status) {
        time_on = (time(NULL) - start);
    } else {
        time_on = 0;
    }

    sprintf(buffer, "3|%i|%i|%i|%i",
            pid, __index, status, (int)time_on);

    write(fd, buffer, MAX_BUF_SIZE);
}

void sighandle_usr2(int sig) {
    // Al ricevimento del segnale, la finestra apre la pipe in lettura e ottiene cosa deve fare.
    // 0|... -> chiudi/apri finestra
    char tmp[MAX_BUF_SIZE];

    read(fd, tmp, MAX_BUF_SIZE);
    char** vars = split(tmp, 2);

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
    // argv = [./window, indice, /tmp/indice];
    pipe_fd = argv[2];
    pid = getpid();
    __index = atoi(argv[1]);

    fd = open(pipe_fd, O_RDWR);

    signal(SIGUSR1, sighandle_usr1);
    signal(SIGUSR2, sighandle_usr2);
    while (1)
        ;

    return 0;
}
