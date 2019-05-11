#ifndef LAUNCHER_H
#define LAUNCHER_H

#include "actions.h"
#include "util.h"

#define HELP_STRING_LAUNCHER \
    "Comandi disponibili:\n\
    user turn shell <pos>	    Per accendere e spegnere la centralina\n\
    switch <id> <label> <pos>   del dispositivo <id> modifica l’interruttore\n\
                                <label> in posizione <pos>, ad esempio:\n\
                                \"switch 3 open on\" imposta per il dispositivo 3\n\
                                l’interruttore “open” su “on” (ad esempio apre una finestra)\n\
    info <id>                   mostra i dettagli del dispositivo\n\
    restart                     ricompila il progetto e riavvia il programma\n"

#define USER_STRING \
    "Sintassi: user turn shell <pos>\n"

void handle_sig(int signal);
void handle_sigint(int signal);
void handle_sighup(int signal);

void switch_launcher(char buf[][MAX_BUF_SIZE], int msgid, int* device_pids);
void info_launcher(char buf[][MAX_BUF_SIZE], int msgid, int* device_pids);
void user_launcher(char buf[][MAX_BUF_SIZE], int msgid, int *device_pids);

void read_msgqueue(int msgid, int*device_pids);

#endif
