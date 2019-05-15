#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../actions.h"
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

    // conto i dispositivi connessi
    int connected = 0;

    int i;
    for (i = 0; i < MAX_HUB_CONNECTED_DEVICES; i++) {
        if (children_pids[i] != -1) {
            connected++;
        }
    }

    sprintf(buffer, "4|%i|%i|%i|%i|!|",
            pid, __index, status, connected);

    // Stampo nel buffer tante volte quanti device ho
    for (i = 0; i < MAX_HUB_CONNECTED_DEVICES; i++) {
        if (children_pids[i] != -1) {
            char* raw_info = get_raw_device_info(children_pids[i]);
            strcat(buffer, raw_info);
            strcat(buffer, "|!|");
            free(raw_info);
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

    if (tmp[0] - '0' == 0) {
        status = !status;

        // DA ESPANDERE
    } else if (tmp[0] - '0' == 1) {
        char** vars = split(tmp);

        // Codice copiato dalla shell
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
                //s printf(__out_buf, "Non c'è più spazio! Rimuovi qualche dispositivo.\n");
                return;
            }
        } else {
            actual_index = device_i - 1;  // compenso per gli array indicizzati a 0
        }

        /// fine partE COPIATA
        __add_ex(vars, actual_index, children_pids);
        free(vars);
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