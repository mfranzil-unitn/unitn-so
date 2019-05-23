#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../util.h"

/* Frigo = 2 */

int shellpid;


int fd;                         /* file descriptor della pipe verso il padre */
int pid, __index, delay, perc, temp; /* variabili di stato */
char log_buf[MAX_BUF_SIZE / 4]; /* buffer della pipe // SE CI SONO PROBLEMI; GUARDA QUI */
int status = 0; /* interruttore accensione */
time_t start, time_on;
key_t key; 
int msgid;


volatile int flag_usr1 = 0;
volatile int flag_usr2 = 0;
volatile int flag_term = 0;

void sighandler_int(int sig) {
    if (sig == SIGUSR1) {
        flag_usr1 = 1;
    }
    if (sig == SIGUSR2) {
        flag_usr2 = 1;
    }
    if (sig == SIGTERM) {
        flag_term = 1;
    }
}

int main(int argc, char* argv[]) {
    /* argv = [./fridge, indice, /tmp/indice]; */
    char tmp[MAX_BUF_SIZE];
    char ppid_pipe[MAX_BUF_SIZE];
    char* this_pipe = NULL; /* nome della pipe */

    char** vars = NULL;
    int ppid, ppid_pipe_fd;

    this_pipe = argv[2];
    pid = getpid();
    __index = atoi(argv[1]);
    fd = open(this_pipe, O_RDWR);

    key = ftok("/tmp/ipc/mqueues", pid);
    msgid = msgget(key, 0666 |IPC_CREAT);

    delay = 15;
    perc = 50;
    temp = 5;

    shellpid = get_shell_pid();
    sprintf(log_buf, "-");

    signal(SIGTERM, sighandler_int);
    signal(SIGUSR1, sighandler_int);
    signal(SIGUSR2, sighandler_int);

    while (1) {
        if (flag_usr1) {
            flag_usr1 = 0;

            if (status) {
                time_on = (time(NULL) - start);
            } else {
                time_on = 0;
            }

            sprintf(tmp, "2|%d|%d|%d|%d|%d|%d|%d|%s",
                    pid, __index, status, (int)time_on, delay, perc, temp, log_buf);
            message.mesg_type = 1;
            sprintf(message.mesg_text, "%s", tmp);
            int rc = msgsnd(msgid, &message, sizeof(message),0);

           // write(fd, tmp, MAX_BUF_SIZE);



            /* Resetto il contenuto del buffer */
            sprintf(log_buf, "-");
        }
        if (flag_usr2) {
            flag_usr2 = 0;
            /* Al ricevimento del segnale, il frigo apre la pipe in lettura e ottiene cosa deve fare. */
            /* 0|... -> chiudi/apri frigo */
            /* 1|TEMP -> setta temperatura del frigo */
            /* 2|DELAY -> setta delay di chiusura */
            /* 3|PERCENT -> setta contenuto */

            read(fd, tmp, MAX_BUF_SIZE);
            vars = split_fixed(tmp, 2);

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
        if (flag_term) {
            /*if ((int)getppid() != shellpid) {
                ppid = (int)getppid();
                kill(ppid, SIGUSR2);
                get_pipe_name(ppid, ppid_pipe); 
                ppid_pipe_fd = open(ppid_pipe, O_RDWR);
                sprintf(tmp, "2|%d", (int)getpid());
                write(ppid_pipe_fd, tmp, sizeof(tmp));
                close(ppid_pipe_fd);
            }*/
            msgctl(msgid, IPC_RMID, NULL);
            exit(0);
        }
        if (status == 1 && start <= time(NULL) - delay) {
            status = 0;
            sprintf(log_buf,
                    "Il frigorifero %d si è chiuso automaticamente dopo %d secondi",
                    __index, delay);
        }
        sleep(1);
    }

    return 0;
}
