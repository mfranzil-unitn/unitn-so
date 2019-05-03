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

// Window = 3

/*
Suppongo che i due interruttori che comandano la finestra Interr_Aperto e
 Interr_Chiuso siano sempre opposti. Quando ne premo uno questo resta in uno
  stato OFF (non può più essere premuto) e l'altro cambia nello stato ON (può essere premuto).

Sia lo switch della centarlina sia lo switch "manuale" modificano lo stato degli interruttori

Con il comando switch permetto solo: switch <id> <Interr_Aperto/Chiuso> ON   	
*/

char* pipe_fd = NULL;
int pid, name, delay;

int status = 0;  // interruttore apertura
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

    sprintf(buffer, "3|%i|%i|%i|%i",
            status, (int)time_on, pid, name);

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
    // argv = [./window, indice, /tmp/indice];
    pipe_fd = argv[2];
    pid = getpid();
    name = atoi(argv[1]);

    signal(SIGUSR1, sighandle_usr1);
    signal(SIGUSR2, sighandle_usr2);
    while (1)
        ;
    return 0;
}
