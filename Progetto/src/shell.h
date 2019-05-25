#ifndef SHELL_H
#define SHELL_H

#include "actions.h"
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

int add_shell(char buf[][MAX_BUF_SIZE], int *device_i, int *children_pids, char *__out_buf);

void cleanup_sig(int sig);
void handle_sig(int sig);
void stop_sig(int sig);
void link_child(int param);

#endif