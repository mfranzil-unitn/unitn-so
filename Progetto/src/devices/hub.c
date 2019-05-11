#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../util.h"

// BULB = 1

int fd;                // file descriptor della pipe verso il padre
char* pipe_fd = NULL;  // nome della pipe

int pid, __index;  // variabili di stato

int status = 0;  // interruttore accensione

int children_pids[MAX_HUB_CONNECTED_DEVICES];
int device_i = 0;

void sighandle_usr1(int sig) {
    // bisogna controllare se i dispositivi sono allineati o meno (override)
    char buffer[MAX_BUF_SIZE];

    sprintf(buffer, "4|%i|%i|%i|%i|",
            pid, __index, status, device_i);

    int i;
    for (i = 0; i < MAX_HUB_CONNECTED_DEVICES; i++) {
        if (children_pids[i] != -1) {
            char** vars = get_device_info(children_pids[i]);

            strcat(buffer, "<");
            int j = 0;
            while (vars[j] != "0") {
                strcat(buffer, vars[j++]);
            }

            strcat(buffer, ">");
            free(vars);
        }
    }

    write(fd, buffer, MAX_BUF_SIZE);
}

void sighandle_usr2(int sig) {
    // Al ricevimento del segnale, la finestra apre la pipe in lettura e ottiene cosa deve fare.
    // 0|.. -> spegni/accendi tutto
    // 1|.. -> attacca contenuto
    char tmp[MAX_BUF_SIZE];

    read(fd, tmp, MAX_BUF_SIZE);
    char** vars = split_fixed(tmp, 2);

    if (atoi(vars[0]) == 0) {
        status = !status;
    } else if (atoi(vars[0]) == 1) {
        char* working_tree = vars[1];
        char** vars = split(working_tree);

        // PARTE COPIAta dalla shell
        device_i++;
        int actual_index = -1;

        if (device_i >= MAX_HUB_CONNECTED_DEVICES) {
            int i;  // del ciclo
            for (i = 0; i < MAX_HUB_CONNECTED_DEVICES; i++) {
                if (children_pids[i] == -1) {
                    actual_index = i;
                    break;
                }
            }
            if (i == MAX_HUB_CONNECTED_DEVICES) {
                // PORCODIO SIAMO NELLA MERDA
                //sprintf(__out_buf, "Non c'è più spazio! Rimuovi qualche dispositivo.\n");
                    return;
            }
        } else {
            actual_index = (device_i) - 1;  // compenso per gli array indicizzati a 0
        }

        /// fine partE COPIATA
        __add_ex(vars, actual_index, children_pids);
    }
}

int main(int argc, char* argv[]) {
    // argv = [./hub, indice, /tmp/indice];
    pipe_fd = argv[2];
    pid = getpid();
    __index = atoi(argv[1]);

    fd = open(pipe_fd, O_RDWR);

    int i;
    for (i = 0; i < MAX_HUB_CONNECTED_DEVICES; i++) {
        children_pids[i] = -1;
    }

    signal(SIGUSR1, sighandle_usr1);
    signal(SIGUSR2, sighandle_usr2);
    while (1)
        ;

    return 0;
}