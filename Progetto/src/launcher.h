#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "util.h"

#define MAX_CHILDREN 100

#define HELP_STRING_LAUNCHER \
    "Comandi disponibili:\n\
	user turn shell <pos>	    Per accendere e spegnere la centralina\n\
    switch <id> <label> <pos>   del dispositivo <id> modifica l’interruttore\n\
                                <label> in posizione <pos>, ad esempio:\n\
                                \"switch 3 open on\" imposta per il dispositivo 3\n\
                                l’interruttore “open” su “on” (ad esempio apre una finestra)\n\
    info <id>                   mostra i dettagli del dispositivo\n\
    restart                     ricompila il progetto e riavvia il programma\n"

#define SWITCH_STRING \
    "Sintassi: switch <id> <label> <pos>\n\
    Interruttori disponibili: bulb: accensione, fridge: temperatura/apertura, window: apertura\n"

#define ADD_STRING \
    "Sintassi: add <device>\nDispositivi disponibili: bulb, window, fridge, hub, timer\n"

void handle_sig(int signal);
void handle_sigint(int signal);
void handle_sighup(int signal);
char* pipename(int pid);
int get_by_index(int in, int* children_pids);
void list(char buf[][MAX_BUF_SIZE], int* children_pids);
void info(char buf[][MAX_BUF_SIZE], int* children_pids);
void __switch(char buf[][MAX_BUF_SIZE], int* children_pids);
void add(char buf[][MAX_BUF_SIZE], int* device_i, int* children_pids);
void ignore_sig(int sig);
void cleanup_sig(int sig);
int n_devices = 0;
int emergencyid;

#endif
