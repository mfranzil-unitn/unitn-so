#include "actions.h"

void __switch(int index, char *action, char *position, int *children_pids) {
    /* Prova a impostare un interruttore ACTION su POSITION di un certo DEVICE */
    char *device_info;
    int pid, fd, status, prova;
    char **vars;
    char pipe_str[MAX_BUF_SIZE];
    char pipe_message[MAX_BUF_SIZE]; /* buffer per la pipe */

    pid = get_device_pid(index, children_pids, &device_info);

    if (pid == -1) {
        printf("Errore! Non esiste questo dispositivo.\n");
        return;
    }

    if (device_info == NULL) {
        printf("Errore di connessione (PID %d)\n", pid);
    }

    vars = split(device_info);
    get_pipe_name(pid, pipe_str); /* Nome della pipe */

    fd = open(pipe_str, O_RDWR);

    if (strcmp(vars[0], BULB_S) == 0) { /* Lampadina */
        if (strcmp(action, "accensione") == 0) {
            status = atoi(vars[3]);
            sprintf(pipe_message, "0|0");
            /*printf("pipe message: %s\n", pipe_message); */

            if (strcmp(position, "on") == 0 && status == 0) {
                write(fd, pipe_message, MAX_BUF_SIZE);
                prova = kill(pid, SIGUSR2); /* sleep(1) */
                if (prova == -1) {
                    printf("ERRORE CHIAMATA\n");
                }
                printf("Lampadina accesa.\n");
            } else if (strcmp(position, "off") == 0 && status == 1) {
                write(fd, pipe_message, MAX_BUF_SIZE);
                kill(pid, SIGUSR2); /* sleep(1) */
                printf("Lampadina spenta.\n");
            } else if (strcmp(position, "off") == 0 && status == 0) { /* Spengo una lampadina spenta */
                printf("Stai provando a spegnere una lampadina spenta!\n");
            } else if (strcmp(position, "on") == 0 && status == 1) { /* Accendo una lampadina accesa */
                printf("Stai provando a accendere una lampadina accesa!\n");
            } else {
                printf("Sintassi non corretta. Sintassi: switch <bulb> accensione <on/off>\n");
            }
        } else {
            printf("Operazione non permessa su una lampadina!\nOperazioni permesse: accensione\n");
        }
    } else if (strcmp(vars[0], FRIDGE_S) == 0) { /* Fridge */
        if (strcmp(action, "apertura") == 0) {
            status = atoi(vars[3]);
            sprintf(pipe_message, "0|0");

            if (strcmp(position, "on") == 0 && status == 0) {
                write(fd, pipe_message, MAX_BUF_SIZE);
                kill(pid, SIGUSR2); /* sleep(1) */
                printf("Frigorifero aperto.\n");
            } else if (strcmp(position, "off") == 0 && status == 1) {
                write(fd, pipe_message, MAX_BUF_SIZE);
                kill(pid, SIGUSR2); /* sleep(1) */
                printf("Frigorifero chiuso.\n");
            } else if (strcmp(position, "off") == 0 && status == 0) { /* Chiudo frigo già chiuso */
                printf("Stai provando a chiudere un frigorifero già chiuso.\n");
            } else if (strcmp(position, "on") == 0 && status == 1) { /* Apro frigo già aperto */
                printf("Stai provando a aprire un frigorifero già aperto.\n");
            } else {
                printf("Sintassi non corretta. Sintassi: switch <fridge> apertura <on/off>\n");
            }
        } else if (strcmp(action, "temperatura") == 0) {
            if ((atoi(position) >= -10 && atoi(position) < 0) || (atoi(position) > 0 && atoi(position) <= 15) || strcmp(position, "0") == 0) {
                sprintf(pipe_message, "1|%s", position);
                write(fd, pipe_message, MAX_BUF_SIZE);
                kill(pid, SIGUSR2); /* sleep(1) */
                printf("Temperatura modificata con successo a %s°C.\n", position);
            } else {
                printf("Sintassi non corretta. Sintassi: switch <fridge> temperatura <-10 - 15>\n");
            }
        } else if (strcmp(action, "delay") == 0) {
            if ((atoi(position) > 0 && atoi(position) <= (60 * 5)) || strcmp(position, "0") == 0) { /* Massimo 5 minuti */
                sprintf(pipe_message, "2|%s", position);
                write(fd, pipe_message, MAX_BUF_SIZE);
                kill(pid, SIGUSR2); /* sleep(1) */
                printf("Tempo di richiusura automatico modificato con successo a %s secondi.\n", position);
            } else {
                printf("Sintassi non corretta. Sintassi: switch <fridge> delay <0-300>.\n");
            }
        } else if (strcmp(action, "riempimento") == 0) { /* Possibile solo manualmente (launcher) */
            if ((atoi(position) > 0 && atoi(position) <= 100) || strcmp(position, "0") == 0) {
                sprintf(pipe_message, "3|%s", position);
                write(fd, pipe_message, MAX_BUF_SIZE);
                kill(pid, SIGUSR2); /* sleep(1) */
                printf("Percentuale di riempimento modificato con successo a %s. \n", position);
            } else {
                printf("Sintassi non corretta. Sintassi: switch <fridge> riempimento <0-100>.\n");
            }
        } else {
            printf("Operazione non permessa su un frigorifero! Operazioni permesse: <temperatura/apertura/delay/riempimento>\n");
        }

    } else if (strcmp(vars[0], WINDOW_S) == 0) { /* Window */
        if (strcmp(action, "apertura") == 0 || strcmp(action, "chiusura") == 0) {
            if (((strcmp(action, "apertura") != 0) || (strcmp(action, "apertura") == 0 && strcmp(position, "off") == 0)) &&
                ((strcmp(action, "chiusura") != 0) || (strcmp(action, "chiusura") == 0 && strcmp(position, "off") == 0))) {
                printf("Operazione non permessa: i pulsanti sono solo attivi!\n");
                /* se off non permetto */
                return;
            }

            status = atoi(vars[3]);
            sprintf(pipe_message, "0|0");

            if (strcmp(action, "apertura") == 0 && status == 0) {
                write(fd, pipe_message, MAX_BUF_SIZE);
                kill(pid, SIGUSR2); /* sleep(1) */
                printf("Finestra aperta.\n");
            } else if (strcmp(action, "chiusura") == 0 && status == 1) {
                write(fd, pipe_message, MAX_BUF_SIZE);
                kill(pid, SIGUSR2); /* sleep(1) */
                printf("Finestra chiusa.\n");
            } else {
                printf("Operazione non permessa: pulsante già premuto.\n");
            }
        } else {
            printf("Operazione non permessa su una finestra! Operazioni permesse: <apertura/chiusura>.");
        }
    } else if (strcmp(vars[0], HUB_S) == 0) { /* Hub */
        if (strcmp(action, "accensione") == 0) {
            status = atoi(vars[3]);
            sprintf(pipe_message, "0|0");

            if (strcmp(position, "on") == 0 && status == 0) {
                write(fd, pipe_message, MAX_BUF_SIZE);
                kill(pid, SIGUSR2); /* sleep(1) */
                printf("Hub acceso.\n");
            } else if (strcmp(position, "off") == 0 && status == 1) {
                write(fd, pipe_message, MAX_BUF_SIZE);
                kill(pid, SIGUSR2); /* sleep(1) */
                printf("Hub spento.\n");
            } else if (strcmp(position, "off") == 0 && status == 0) { /* Spengo un hub spento */
                printf("Stai provando a spegnere un hub spenta!\n");
            } else if (strcmp(position, "on") == 0 && status == 1) { /* Accendo un hub acceso */
                printf("Stai provando a accendere un hub acceso!\n");
            } else {
                printf("Sintassi non corretta. Sintassi: switch <hub> accensione <on/off>\n");
            }
        } else {
            printf("Operazione non permessa su un hub!\nOperazioni permesse: accensione\n");
        }
    } else if (strcmp(vars[0], TIMER_S) == 0) {
        if (strcmp(action, "orario") == 0) {
            int h_start, m_start, h_end, m_end;
            /* Aggiungere controlli sugli orari */
            int scan = sscanf(position, "%d:%d->%d:%d", &h_start, &m_start, &h_end, &m_end);
            if (scan != 4 || h_start < 0 || h_start > 23 || h_end < 0 || h_end > 59 || h_start > h_end || m_start > m_end) {
                printf("Formattazione degli orari sbagliata. Formato (24 ore): \"HH:MM -> HH:MM\"\n");
            } else {
                sprintf(pipe_message, "0|%d|%d|%d|%d", h_start, m_start, h_end, m_end);
                write(fd, pipe_message, MAX_BUF_SIZE);
                kill(pid, SIGUSR2);
                printf("Timer settato dalle ore %d:%d alle ore %d:%d\n", h_start, m_start, h_end, m_end);
            }
        } else {
            printf("Operazione non permessa su un hub!\nOperazioni permesse: orario\n");
        }
    } else { /* tutti gli altri dispositivi */
        printf("Dispositivo non supportato.\n");
    }
    close(fd);
    free(vars);
}

void __info(int index, int *children_pids) {
    char *info = NULL;
    char **info_p = NULL;

    int pid;
    
    pid = get_device_pid(index, children_pids, &info);

    if (pid == -1) {
        printf("Errore! Non esiste questo dispositivo.\n");
        return;
    }

    if (info == NULL) {
        printf("Errore di connessione (PID %d)\n", pid);
    }

    if (strncmp(info, HUB_S, 1) != 0) {
        info_p = split(info);
        __print(info_p);
    } else {
        hub_tree_parser(info);
    }

    free(info);
}

void __print(char **vars) {
    if (strcmp(vars[0], BULB_S) == 0) {
        /* Lampadina - parametri: tipo, pid, indice, stato, tempo di accensione */
        printf("Oggetto: Lampadina\nPID: %s\nIndice: %s\nStato: %s\nTempo di accensione: %s\n",
               vars[1], vars[2], atoi(vars[3]) ? "ON" : "OFF", vars[4]);
    } else if (strcmp(vars[0], FRIDGE_S) == 0) {
        /* Frigo -  parametri: tipo, pid, indice, stato, tempo di apertura, delay */
        /* percentuale riempimento, temperatura interna */
        printf("Oggetto: Frigorifero\nMessaggio di log: <%s>\n", vars[8]);
        printf("PID: %s\nIndice: %s\nStato: %s\nTempo di apertura: %s sec\n",
               vars[1], vars[2], atoi(vars[3]) ? "Aperto" : "Chiuso", vars[4]);
        printf("Delay richiusura: %s sec\nPercentuale riempimento: %s\nTemperatura: %s°C\n",
               vars[5], vars[6], vars[7]);
    } else if (strcmp(vars[0], WINDOW_S) == 0) {
        /* Finestra - parametri: tipo, pid, indice, stato, tempo di accensione */
        printf("Oggetto: Finestra\nPID: %s\nIndice: %s\nStato: %s\nTempo di apertura: %s sec\n",
               vars[1], vars[2], atoi(vars[3]) ? "Aperto" : "Chiuso", vars[4]);
    } else if (strcmp(vars[0], HUB_S) == 0) {
        printf("Oggetto: Hub\nPID: %s\nIndice: %s\nStato: %s\nDispositivi collegati: %s\n",
               vars[1], vars[2], atoi(vars[3]) ? "Acceso" : "Spento", vars[4]);
    } else if (strcmp(vars[0], TIMER_S) == 0) {
        printf("Oggetto: Timer\nPID: %s\nIndice: %s\nStato: %s\nFascia oraria: %s:%s -> %s:%s\nDispositivi collegati: -\n",
               vars[1], vars[2], atoi(vars[3]) ? "Acceso" : "Spento", vars[4], vars[5], vars[6], vars[7]);
    } else {
        printf("Dispositivo non supportato.\n");
    }
}

int __add(char *device, int device_index, int *children_pids, char *__out_buf) {
    int actual_index = -1;
    pid_t pid;
    int i; /* del ciclo */
    char pipe_str[MAX_BUF_SIZE];
    char index_str[MAX_BUF_SIZE / 4];
    char program_name[MAX_BUF_SIZE / 4];
    char device_name[MAX_BUF_SIZE];

    for (i = 0; i < MAX_CHILDREN; i++) {
        if (children_pids[i] == -1) {
            actual_index = i;
            break;
        }
    }

    if (i == MAX_CHILDREN) {
        sprintf(__out_buf, "Non c'è più spazio! Rimuovi qualche dispositivo.\n");
        return 0;
    }

    pid = fork();
    if (pid == 0) { /* Figlio */
        /* Apro una pipe per padre-figlio */
        get_pipe_name(getpid(), pipe_str);
        mkfifo(pipe_str, 0666);
        /* Conversione a stringa dell'indice */
        sprintf(index_str, "%d", device_index);
        sprintf(program_name, "./%s%s", DEVICES_POSITIONS, device);
        execlp(program_name, program_name, index_str, pipe_str, NULL);
        exit(0);
    } else { /* Padre */
        children_pids[actual_index] = pid;
        get_device_name_str(device, device_name);
        sprintf(__out_buf, "Aggiunto un dispositivo di tipo %s con PID %d e indice %d\n",
                device_name, pid, device_index);
    }
    return 1;
    /*  return 1; */
}

void __list(int *children_pids) {
    /* prende come input l'indice/nome del dispositivo, ritorna il PID */
    int i;
    char **vars = NULL;
    char *tmp;
    int children_pid;
    char device_name[MAX_BUF_SIZE];

    /* Centralina */
    printf("Centralina, (PID %d, Indice 0)\n", (int)getpid());

    for (i = 0; i < MAX_CHILDREN; i++) { /* l'indice i è logicamente indipendente dal nome/indice del dispositivo */
        children_pid = children_pids[i];
        if (children_pid != -1) {
            printf("∟ ");
            tmp = get_raw_device_info(children_pid);

            if (tmp == NULL) {
                printf("Errore di comunicazione (PID %d)\n", children_pid);
            } else {
                if (strncmp(tmp, HUB_S, 1) == 0) {
                    hub_tree_parser(tmp);
                } else {
                    vars = split(tmp);
                    get_device_name(atoi(vars[0]), device_name);
                    device_name[0] += 'A' - 'a';

                    printf("%s, (PID %s, Indice %s)\n", device_name, vars[1], vars[2]);
                    /* Pulizia */
                    free(vars);
                }
            }
        }
    }
}

void __del(int index, int *children_pids, char *__out_buf) {
    char *raw_device_info = NULL;
    char **vars;
    int pid;
    char pipe_str[MAX_BUF_SIZE];
    char device_name[MAX_BUF_SIZE];
    int i;

    pid = get_device_pid(index, children_pids, &raw_device_info);

    if (pid == -1) {
        sprintf(__out_buf, "Errore! Non esiste questo dispositivo.\n");
        return;
    }

    vars = split(raw_device_info);

    get_pipe_name(pid, pipe_str);

    get_device_name(atoi(vars[0]), device_name);
    device_name[0] += 'A' - 'a';

    sprintf(__out_buf, "Dispositivo di tipo %s con PID %s e indice %s rimosso.\n",
            device_name, vars[1], vars[2]);

    free(vars);

    kill(pid, SIGTERM);
    /*kill(pid, 9);      // da modificare con un comando opportuno... */
    remove(pipe_str); /* RIP pipe */

    for (i = 0; i < MAX_CHILDREN; i++) {
        if (children_pids[i] == pid) {
            children_pids[i] = -1;
            return;
        }
    }
}

void __link(int index, int controller, int *children_pids) {
    char *raw_device_info = NULL;
    char *raw_controller_info = NULL;
    int device_pid, controller_pid, fd;
    char buf[MAX_BUF_SIZE];
    char __out_buf[MAX_BUF_SIZE];
    char controller_pipe_name[MAX_BUF_SIZE];

    device_pid = get_device_pid(index, children_pids, &raw_device_info);

    if (device_pid == -1) {
        printf("Errore! Non esiste il dispositivo %d.\n", index);
        return;
    }

    if (raw_device_info == NULL) {
        printf("Errore in lettura del dispositivo.\n");
        return;
    }

    controller_pid = get_device_pid(controller, children_pids, &raw_controller_info);

    if (controller_pid == -1) {
        printf("Errore! Non esiste il dispositivo %d.\n", controller);
        return;
    }

    if (device_pid == controller_pid) {
        printf("Errore! Non puoi collegarti a te stesso.\n");
        return;
    }

    if (is_controller(controller_pid, raw_controller_info)) {
        if (!hub_is_full(controller_pid, raw_controller_info)) {
            sprintf(buf, "1|");
            strcat(buf, raw_device_info);
            free(raw_device_info);

            __del(index, children_pids, __out_buf);

            get_pipe_name(controller_pid, controller_pipe_name);

            fd = open(controller_pipe_name, O_RDWR);
            write(fd, buf, MAX_BUF_SIZE);
            kill(controller_pid, SIGUSR2);

            printf("Spostato l'oggetto %d sotto l'oggetto %d\n", index, controller);
            close(fd);
        } else {
            printf("Operazione non permessa. L'hub %d è già pieno. Eliminare qualche dispositivo.\n", controller);
        }
    } else {
        printf("Configurazione dei dispositvi non valida. Sintassi: link <device> to <hub/timer>\n");
    }
}

void __add_ex(char **vars, int *children_pids) {
    char __out_buf[MAX_BUF_SIZE];
    if (strcmp(vars[0], BULB_S) == 0) { /* Lampadina */
        __add("bulb", atoi(vars[2]), children_pids, __out_buf);
        /* Chiaramente minchia posso replicare il time_on o lo stato... */
        /* se scollego una lampadina quella si spegne */
    } else if (strcmp(vars[0], FRIDGE_S) == 0) { /* Frigo */
        __add("fridge", atoi(vars[2]), children_pids, __out_buf);
        /*    .... aggiungere la temperatura eventualmente riempimento etc */
    } else if (strcmp(vars[0], WINDOW_S) == 0) { /* Frigo */
        __add("window", atoi(vars[2]), children_pids, __out_buf);
    } else if (strcmp(vars[0], HUB_S) == 0) {
        __add("hub", atoi(vars[2]), children_pids, __out_buf);
    } else {
        printf("Da implementare...");
    }
}

int hub_tree_constructor(char *__buf, int *children_pids) {
    char *tokenizer = strtok(__buf, "|");
    char *old = NULL;
    /*printf("Level %d\n", level); */

    /* DISPOSITIVO; PID; ID */

    char **vars = malloc((FRIDGE_PARAMETERS + 4) * sizeof(*vars));
    int i = 0;
    int to_be_added = 1;

    while (tokenizer != NULL) {
        if (strcmp(tokenizer, "<!") == 0) {
            to_be_added += atoi(old) - 1;
            i = 0;
            __add_ex(vars, children_pids);
            /* segnarsi chi è il padre e poi fare link */
        } else if (strcmp(tokenizer, "!>") == 0) {
            if (strcmp(old, "<!") == 0 && to_be_added > 0) {
                i = 0;

                __add_ex(vars, children_pids);
                to_be_added--;
            }
        } else if (strcmp(tokenizer, "!") == 0) {
            if (to_be_added > 0) {
                i = 0;

                __add_ex(vars, children_pids);
                to_be_added--;
            }
        } else {
            vars[i++] = tokenizer;
            /*printf("%s, ", tokenizer); */
        }
        old = tokenizer;
        tokenizer = strtok(NULL, "|");
    }
    free(vars);
    return -1;
}

int __link_ex(int son_pid, int parent_pid, int shellpid) {
    int controller;
    char *tmp_controller;
    char **parent_info;
    char buf[MAX_BUF_SIZE];
    char tmp[1024];
    char *son_info;
    char **son_info_split;
    int index;
    char controller_pipe_name[MAX_BUF_SIZE];
    int fd;

    son_info = get_raw_device_info(son_pid);

    if (son_info == NULL) {
        printf("Errore di comunicazione (PID: %d)\n", son_pid);
        return -1;
    }

    if (parent_pid != shellpid) {
        tmp_controller = get_raw_device_info(parent_pid);

        if (tmp_controller == NULL) {
            printf("Errore di comunicazione (PID: %d)\n", parent_pid);
            return -1;
        }

        parent_info = split(tmp_controller);
        controller = atoi(parent_info[2]);
    } else {
        controller = 0;
    }

    /*printf("Getting son info\n"); */
    sprintf(buf, "1|%s", son_info);
    printf("Buf: %s\n", buf);
    sprintf(tmp, "%s", son_info);
    son_info_split = split(tmp);
    index = atoi(son_info_split[2]);

    /*Deleting son */
    /*printf("Deleting\n"); */
    kill(son_pid, SIGTERM);

    get_pipe_name(parent_pid, controller_pipe_name);

    /*printf("Killing %d\n", parent_pid); */
    kill(parent_pid, SIGUSR2); /* sleep(1) */
    fd = open(controller_pipe_name, O_RDWR);
    write(fd, buf, MAX_BUF_SIZE);

    printf("Spostato l'oggetto %d sotto l'oggetto %d\n", index, controller);
    /*close(fd); */

    free(son_info);

    return 1;
}

/* int __link_ex(int *son_pids, int parent_pid, int shellpid) {
    int controller;
    char buf[MAX_BUF_SIZE];
    int count = 0;
    int i = 0;
    char *tmp_controller;
    char **parent_info;

    int son_pid;
    char tmp[MAX_BUF_SIZE];
    char **son_info;
    int index;

    char buffer[MAX_BUF_SIZE + 24];
    char controller_pipe_name[MAX_BUF_SIZE];

    int fd;

    printf("LINK_EX\n");

    if (parent_pid != shellpid) {
        tmp_controller = get_raw_device_info(parent_pid);
        parent_info = split(tmp_controller);
        controller = atoi(parent_info[2]);
    } else {
        controller = 0;
    }

    sprintf(buf, "-");
    for (i = 0; i < MAX_CHILDREN; i++) {
        printf("SON: %d\n", son_pids[i]);
        if (son_pids[i] != -1) {
            count++;
            son_pid = son_pids[i];
            lprintf("DEBUG: Getting son: %d info\n", son_pid);
            sprintf(tmp, "%s-", get_raw_device_info(son_pid));
            lprintf("DEBUG: TMP_Link: %s\n", tmp);
            strcat(buf, tmp);
            lprintf("DEBUG: BUF of %d: %s", son_pid, buf);
            son_info = split(tmp);
            index = atoi(son_info[2]);

            lprintf("DEBUG: Deleting\n");
            kill(son_pid, SIGTERM);
            lprintf("DEBUG: Spostando l'oggetto %d sotto l'oggetto %d\n", index, controller);
        }
    }
    printf("Exiting For\n");
    sprintf(buffer, "%d%s", count, buf);
    get_pipe_name(parent_pid, controller_pipe_name);
    lprintf("DEBUG: Killing %d\n", parent_pid);
    kill(parent_pid, SIGUSR2);
    fd = open(controller_pipe_name, O_RDWR);
    write(fd, buffer, MAX_BUF_SIZE);
    close(fd);

    return 1;
}*/