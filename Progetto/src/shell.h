#ifndef SHELL_H
#define SHELL_H

#include <sys/ipc.h> 
#include <sys/msg.h> 
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "util.h"


#define HELP_STRING \
    "Comandi disponibili:\n\
    list                        elenca tutti i dispositivi (quelli \n\
                                disponibili con un nome, quelli attivi\n\
                                anche con un “id” univoco per ciascuno \n\
                                e inoltre ne riepiloga le caratteristiche)\n\
    add <device>                aggiunge un <device> al sistema e ne\n\
                                mostra i dettagli (es. “add bulb”)\n\
    del <id>                    rimuove il dispositivo <id>: se è di controllo\n\
                                rimuove anche i dispositivi sottostanti\n\
    link <id> to <id>           collega i due dispositivi tra loro (almeno uno\n\
                                dei due dev’essere di controllo: controller, hub o timer)\n\
    switch <id> <label> <pos>   del dispositivo <id> modifica l’interruttore\n\
                                <label> in posizione <pos>, ad esempio:\n\
                                \"switch 3 open on\" imposta per il dispositivo 3\n\
                                l’interruttore “open” su “on” (ad esempio apre una finestra)\n\
    info <id>                   mostra i dettagli del dispositivo\n"

#define SWITCH_STRING \
    "Sintassi: switch <id> <label> <pos>\n\
    Interruttori disponibili: bulb: accensione, fridge: temperatura/apertura, window: apertura\n"

#define ADD_STRING \
    "Sintassi: add <device>\nDispositivi disponibili: bulb, window, fridge, hub, timer\n"

#define DEL_STRING \
    "Sintassi del <device>\n"

void handle_sig(int signal);
char* pipename(int pid);
int get_by_index(int in, int* children_pids);
void list(char buf[][MAX_BUF_SIZE], int* children_pids);
void info(char buf[][MAX_BUF_SIZE], int* children_pids);
void __switch(char buf[][MAX_BUF_SIZE], int* children_pids);
void add(char buf[][MAX_BUF_SIZE], int* device_i, int* children_pids);
void ignore_sig(int sig);
void cleanup_sig(int sig);

#endif