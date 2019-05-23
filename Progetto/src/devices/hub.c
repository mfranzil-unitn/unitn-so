#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../actions.h"
#include "../util.h"

/* HUB = 4 */

int shellpid;

int fd;           /* file descriptor della pipe verso il padre */
int pid, __index; /* variabili di stato */
int status = 0;   /* interruttore accensione */

int children_pids[MAX_CHILDREN];
int override = 0;

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
/*Itera sui figli, in realtà fino a MAX_CHILDREN, e controlla che gli stati siano congruenti. */
/*E modifica il vettore over_index Maschera di bit. */
int check_override(int* over_index) {
    int i = 0;
    int ret = 0;
    char** vars;

    for (i = 0; i < MAX_CHILDREN; i++) {
        if (children_pids[i] != -1) {
            char* raw_info = get_raw_device_info(children_pids[i]);

            if (raw_info == NULL) {
                continue;
            }

            vars = split(raw_info);
            if (atoi(vars[3]) != status) {
                over_index[i] = 1;
                ret = 1;
            }
        }
    }

    free(vars);

    return ret;
}

int main(int argc, char* argv[]) {
    /* argv = [./hub, indice, /tmp/indice]; */
    char tmp[MAX_BUF_SIZE];
    char* this_pipe = NULL; /* nome della pipe */
    
    int i;

    int connected;

    this_pipe = argv[2];
    pid = getpid();
    __index = atoi(argv[1]);
    fd = open(this_pipe, O_RDWR);

    for (i = 0; i < MAX_CHILDREN; i++) {
        children_pids[i] = -1;
    }

    shellpid = get_shell_pid();

    signal(SIGCHLD, SIG_IGN);
    signal(SIGTERM, sighandler_int);
    signal(SIGUSR1, sighandler_int);
    signal(SIGUSR2, sighandler_int);

    while (1) {
        if (flag_usr1) {
            flag_usr1 = 0;
            /*printf("hub usr1: %d\n", pid); */
            /* bisogna controllare se i dispositivi sono allineati o meno (override) */

            /* conto i dispositivi connessi */
            connected = 0;

            for (i = 0; i < MAX_CHILDREN; i++) {
                if (children_pids[i] != -1) {
                    connected++;
                }
            }

            sprintf(tmp, "4|%i|%i|%i|%i|<!|",
                    pid, __index, status, connected);

            /* Stampo nel buffer tante volte quanti device ho */
            for (i = 0; i < MAX_CHILDREN; i++) {
                if (children_pids[i] != -1) {
                    char* raw_info = get_raw_device_info(children_pids[i]);
                    strcat(tmp, raw_info);
                    strcat(tmp, "|!|");
                    free(raw_info);
                }
            }

            strcat(tmp, "!>");
            write(fd, tmp, MAX_BUF_SIZE);
        }
        if (flag_usr2) {
            flag_usr2 = 0;
            /* La finestra apre la pipe in lettura e ottiene cosa deve fare. */
            /* 0|.. -> spegni/accendi tutto */
            /* 1|.. -> attacca contenuto */
            /* 2|.. -> toglie contenuto */

            /*printf("hub usr2: %d\n", pid); */

            char* tmp = malloc(MAX_BUF_SIZE * sizeof(tmp));
            int over_index[MAX_CHILDREN];
            read(fd, tmp, MAX_BUF_SIZE);
            /*printf("End Read: %s\n\n", tmp); */
            int code = tmp[0] - '0';
            /*printf("hub code: %d\n", code); */

            for (i = 0; i < MAX_CHILDREN; i++) {
                over_index[i] = 0;
            }

            /*Valore che indica lo stato di override o meno. Al MOMENTO INCARTAT TUTTO BOIA. */
            /*override = check_override(over_index); */

            if (code == 0) {
                /*printf("CODE 0\n"); */
                status = !status;
                int i = 0;
                char* pipe_str;
                for (i = 0; i < MAX_CHILDREN; i++) {
                    if (children_pids[i] != -1 && !over_index[i]) {
                        char* pos = "on";
                        if (status) {
                            pos = "off";
                        }
                        __switch(children_pids[i], "accensione", pos, children_pids);
                    }
                }
                free(tmp);
            }
            if (code == 1) {
                /*printf("CODE 1\n"); */
                tmp = tmp + 2;
                char** vars = split(tmp);
                __add_ex(vars, children_pids);
                free(vars);
                free(tmp - 2);
            }
            if (code == 2) {
                /*printf("CODE 2\n"); */
                char** vars = split(tmp);
                int j = 0;
                for (j = 0; j < MAX_CHILDREN; j++) {
                    if (children_pids[j] == atoi(vars[1])) {
                        /*printf("BECCATO: childern_Pids: %d, atoi: %d\n", children_pids[j], atoi(vars[1])); */
                        children_pids[j] = -1;
                    }
                }
            }
        }
        if (flag_term) {
            int done = 1;
            int ppid = (int)getppid();
            if (ppid != shellpid) {
                kill(ppid, SIGUSR2);
                char pipe_str[MAX_BUF_SIZE];
                get_pipe_name(ppid, pipe_str); /* Nome della pipe */
                int fd = open(pipe_str, O_RDWR);
                char tmp[MAX_BUF_SIZE];
                sprintf(tmp, "2|%d", (int)getpid());
                write(fd, tmp, sizeof(tmp));
            }

            for (i = 0; i < MAX_CHILDREN; i++) {
                if (children_pids[i] != -1) {
                    printf("Chiamata link_ex per figlio %d\n", children_pids[i]);
                    int ret = __link_ex(children_pids[i], ppid, shellpid);
                    if (ret != 1) {
                        done = 0;
                    }
                }
            }
            if (done) {
                exit(0);
            } else {
                printf("Errore nell'eliminazione\n");
            }
        }
        sleep(10);
    }

    return 0;
}
