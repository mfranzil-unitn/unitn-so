#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../actions.h"
#include "../util.h"

// HUB = 4

int fd;                // file descriptor della pipe verso il padre
char* pipe_fd = NULL;  // nome della pipe

int pid, __index;  // variabili di stato

int status = 0;  // interruttore accensione

int children_pids[MAX_CHILDREN];
int override = 0;

void sighandle_usr1(int sig) {
    // bisogna controllare se i dispositivi sono allineati o meno (override)
    char buffer[MAX_BUF_SIZE];

    // conto i dispositivi connessi
    int connected = 0;

    int i;
    for (i = 0; i < MAX_CHILDREN; i++) {
        if (children_pids[i] != -1) {
            connected++;
        }
    }

    sprintf(buffer, "4|%i|%i|%i|%i|<!|",
            pid, __index, status, connected);

    // Stampo nel buffer tante volte quanti device ho
    for (i = 0; i < MAX_CHILDREN; i++) {
        if (children_pids[i] != -1) {
            char* raw_info = get_raw_device_info(children_pids[i]);
            strcat(buffer, raw_info);
            strcat(buffer, "|!|");
            free(raw_info);
        }
    }

    strcat(buffer, "!>");

    write(fd, buffer, MAX_BUF_SIZE);
}

//Itera sui figli, in realtà fino a MAX_CHILDREN, e controlla che gli stati siano congruenti.
//E modifica il vettore over_index Maschera di bit.
int check_override(int* over_index) {
    int i = 0;
    int ret = 0;
    for (i = 0; i < MAX_CHILDREN; i++) {
        if (children_pids[i] != -1) {
            char** vars = get_device_info(children_pids[i]);
            if (atoi(vars[3]) != status) {
                over_index[i] = 1;
                ret = 1;
            }
        }
    }
    return ret;
}

void sighandle_usr2(int sig) {
    // Al ricevimento del segnale, la finestra apre la pipe in lettura e ottiene cosa deve fare.
    // 0|.. -> spegni/accendi tutto
    // 1|.. -> attacca contenuto
    char* tmp = malloc(MAX_BUF_SIZE * sizeof(tmp));
    int over_index[MAX_CHILDREN];
    read(fd, tmp, MAX_BUF_SIZE);

    //Valore che indica lo stato di override o meno.
    override = check_override(over_index);

    if (tmp[0] - '0' == 0) {
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
    } else if (tmp[0] - '0' == 1) {
        tmp = tmp + 2;
        char** vars = split(tmp);
        __add_ex(vars, children_pids);
        free(vars);
        free(tmp - 2);
    }
}

int main(int argc, char* argv[]) {
    // argv = [./hub, indice, /tmp/indice];
    pipe_fd = argv[2];
    pid = getpid();
    __index = atoi(argv[1]);

    fd = open(pipe_fd, O_RDWR);

    int i;
    for (i = 0; i < MAX_CHILDREN; i++) {
        children_pids[i] = -1;
    }

    signal(SIGUSR1, sighandle_usr1);
    signal(SIGUSR2, sighandle_usr2);
    while (1)
        ;

    return 0;
}