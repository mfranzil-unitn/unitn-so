#include "actions.h"

void __switch(int device, char *action, char *position, int *children_pids) {
    // Prova a impostare un interruttore ACTION su POSITION di un certo DEVICE
    int pid = get_device_pid(device, children_pids);

    if (pid == -1) {
        cprintf("Errore! Non esiste questo dispositivo.\n");
        return;
    }

    char **vars = get_device_info(pid);


    char pipe_str[MAX_BUF_SIZE];
    get_pipe_name(pid, pipe_str);  // Nome della pipe

    char pipe_message[MAX_BUF_SIZE];  // buffer per la pipe

    int fd = open(pipe_str, O_RDWR);

    if (strcmp(vars[0], BULB_S) == 0) {  // Lampadina
        if (strcmp(action, "accensione") == 0) {
            int status = atoi(vars[3]);
            sprintf(pipe_message, "0|0");

            if (strcmp(position, "on") == 0 && status == 0) {
                write(fd, pipe_message, MAX_BUF_SIZE);
                kill(pid, SIGUSR2);
                cprintf("Lampadina accesa.\n");
            } else if (strcmp(position, "off") == 0 && status == 1) {
                write(fd, pipe_message, MAX_BUF_SIZE);
                kill(pid, SIGUSR2);
                cprintf("Lampadina spenta.\n");
            } else if (strcmp(position, "off") == 0 && status == 0) {  // Spengo una lampadina spenta
                cprintf("Stai provando a spegnere una lampadina spenta!\n");
            } else if (strcmp(position, "on") == 0 && status == 1) {  // Spengo una lampadina accesa
                cprintf("Stai provando a accendere una lampadina accesa!\n");
            } else {
                cprintf("Sintassi non corretta. Sintassi: switch <bulb> accensione <on/off>\n");
            }
        } else {
            cprintf("Operazione non permessa su una lampadina!\nOperazioni permesse: accensione\n");
        }
    } else if (strcmp(vars[0], FRIDGE_S) == 0) {  // Fridge
        if (strcmp(action, "apertura") == 0) {
            int status = atoi(vars[3]);
            sprintf(pipe_message, "0|0");

            if (strcmp(position, "on") == 0 && status == 0) {
                write(fd, pipe_message, MAX_BUF_SIZE);
                kill(pid, SIGUSR2);
                cprintf("Frigorifero aperto.\n");
            } else if (strcmp(position, "off") == 0 && status == 1) {
                write(fd, pipe_message, MAX_BUF_SIZE);
                kill(pid, SIGUSR2);
                cprintf("Frigorifero chiuso.\n");
            } else if (strcmp(position, "off") == 0 && status == 0) {  // Chiudo frigo già chiuso
                cprintf("Stai provando a chiudere un frigorifero già chiuso.\n");
            } else if (strcmp(position, "on") == 0 && status == 1) {  // Apro frigo già aperto
                cprintf("Stai provando a aprire un frigorifero già aperto.\n");
            } else {
                cprintf("Sintassi non corretta. Sintassi: switch <fridge> apertura <on/off>\n");
            }
        } else if (strcmp(action, "temperatura") == 0) {
            sprintf(pipe_message, "1|%s", position);

            write(fd, pipe_message, MAX_BUF_SIZE);
            kill(pid, SIGUSR2);
            cprintf("Temperatura modificata con successo a %s°C.\n", position);
        } else {
            cprintf("Operazione non permessa su un frigorifero! Operazioni permesse: <temperatura/apertura>\n");
        }
    } else if (strcmp(vars[0], WINDOW_S) == 0) {  // Window
        if (((strcmp(action, "apertura") != 0) || (strcmp(action, "apertura") == 0 && strcmp(position, "off") == 0)) &&
            ((strcmp(action, "chiusura") != 0) || (strcmp(action, "chiusura") == 0 && strcmp(position, "off") == 0))) {
            cprintf("Operazione non permessa: i pulsanti sono solo attivi!\n");
            // se off non permetto
            return;
        }

        int status = atoi(vars[3]);
        sprintf(pipe_message, "0|0");

        if (strcmp(action, "apertura") == 0 && status == 0) {
            write(fd, pipe_message, sizeof(pipe_message));
            kill(pid, SIGUSR2);
            cprintf("Finestra aperta.\n");
        } else if (strcmp(action, "chiusura") == 0 && status == 1) {
            write(fd, pipe_message, sizeof(pipe_message));
            kill(pid, SIGUSR2);
            cprintf("Finestra chiusa.\n");
        } else {
            cprintf("Operazione non permessa: pulsante già premuto.\n");
        }
    } else {  // tutti gli altri dispositivi
        cprintf("Dispositivo non supportato.\n");
    }
    close(fd);
    free(vars);
}

void __info(int index, int *children_pids) {
    int pid = get_device_pid(index, children_pids);

    if (pid == -1) {
        cprintf("Errore! Non esiste questo dispositivo.\n");
        return;
    }

    char **vars = get_device_info(pid);

    if (strcmp(vars[0], "1") == 0) {
        // Lampadina - parametri: tipo, pid, indice, stato, tempo di accensione

        cprintf("Oggetto: Lampadina\nPID: %s\nIndice: %s\nStato: %s\nTempo di accensione: %s\n",
                vars[1], vars[2], atoi(vars[3]) ? "ON" : "OFF", vars[4]);
    } else if (strcmp(vars[0], "2") == 0) {
        // Frigo -  parametri: tipo, pid, indice, stato, tempo di apertura, delay
        // percentuale riempimento, temperatura interna

        cprintf("Oggetto: Frigorifero\nMessaggio di log: <%s>\n", vars[8]);
        cprintf("PID: %s\nIndice: %s\nStato: %s\nTempo di apertura: %s sec\n",
                vars[1], vars[2], atoi(vars[3]) ? "Aperto" : "Chiuso", vars[4]);
        cprintf("Delay richiusura: %s sec\nPercentuale riempimento: %s\nTemperatura: %s°C\n",
                vars[5], vars[6], vars[7]);
    } else if (strcmp(vars[0], "3") == 0) {
        // Finestra - parametri: tipo, pid, indice, stato, tempo di accensione
        cprintf("Oggetto: Finestra\nPID: %s\nIndice: %s\nStato: %s\nTempo di apertura: %s sec\n",
                vars[1], vars[2], atoi(vars[3]) ? "Aperto" : "Chiuso", vars[4]);
    } else if (strcmp(vars[0], "4") == 0) {
        // Hub -  parametri: tipo, pid, stato indice
        cprintf("Oggetto: Hub\nPID: %s\nIndice: %s\nStato: %s\n",
                vars[1], vars[2], atoi(vars[3]) ? "ON" : "OFF");
    } else {
        cprintf("Dispositivo non supportato.\n");
    }
    free(vars);
}

int __add(char *device, int *device_i, int *children_pids, char *__out_buf) {
    // Aumento l'indice progressivo dei dispositivi
    (*device_i)++;
    int actual_index = -1;

    if (*device_i >= MAX_CHILDREN) {
        int i;  // del ciclo
        for (i = 0; i < MAX_CHILDREN; i++) {
            if (children_pids[i] == -1) {
                actual_index = i;
                break;
            }
        }
        if (i == MAX_CHILDREN) {
            sprintf(__out_buf, "Non c'è più spazio! Rimuovi qualche dispositivo.\n");
            return;
        }
    } else {
        actual_index = *device_i - 1;  // compenso per gli array indicizzati a 0
    }

    pid_t pid = fork();
    if (pid == 0) {  // Figlio
        // Apro una pipe per padre-figlio
        char pipe_str[MAX_BUF_SIZE];
        get_pipe_name(getpid(), pipe_str);
        mkfifo(pipe_str, 0666);

        // Conversione a stringa dell'indice
        char *index_str = malloc(4 * sizeof(char));
        sprintf(index_str, "%d", *device_i);

        char program_name[MAX_BUF_SIZE / 4];
        sprintf(program_name, "./%s%s", DEVICES_POSITIONS, device);

        // Metto gli argomenti in un array e faccio exec
        char *const args[] = {program_name, index_str, pipe_str, NULL};
        execvp(args[0], args);

        free(index_str);
        exit(0);
    } else {  // Padre
        children_pids[actual_index] = pid;

        char device_name[MAX_BUF_SIZE];
        get_device_name_str(device, device_name);

        sprintf(__out_buf, "Aggiunto un dispositivo di tipo %s con PID %i e indice %i\n",
                device_name, pid, *device_i);
    }
    return 1;
}

void __list(int *children_pids) {
    // prende come input l'indice/nome del dispositivo, ritorna il PID
    char *pipe_str = NULL;

    int i;
    for (i = 0; i < MAX_CHILDREN; i++) {  // l'indice i è logicamente indipendente dal nome/indice del dispositivo
        int children_pid = children_pids[i];
        char tmp[MAX_BUF_SIZE];

        if (children_pid == -1) {
            continue;  // dispositivo non più nei figli
        }

        char **vars = get_device_info(children_pid);

        char device_name[MAX_BUF_SIZE];
        get_device_name(atoi(vars[0]), device_name);
        device_name[0] += 'A' - 'a';

        cprintf("Dispositivo: %s, PID %s, nome %s\n", device_name, vars[1], vars[2]);
        // Pulizia
        free(vars);
    }
}

void __del(int index, int *children_pids, char *__out_buf) {
    int pid = get_device_pid(index, children_pids);

    if (pid == -1) {
        sprintf(__out_buf, "Errore! Non esiste questo dispositivo.\n");
        return;
    }

    char **vars = get_device_info(pid);

    char pipe_str[MAX_BUF_SIZE];
    get_pipe_name(pid, pipe_str);

    char device_name[MAX_BUF_SIZE];
    get_device_name(atoi(vars[0]), device_name);
    device_name[0] += 'A' - 'a';

    sprintf(__out_buf, "Dispositivo di tipo %s con PID %s e indice %s rimosso.\n",
            device_name, vars[1], vars[2]);

    free(vars);

    kill(pid, 9);      // da modificare con un comando opportuno...
    remove(pipe_str);  // RIP pipe

    int i;
    for (i = 0; i < MAX_CHILDREN; i++) {
        if (children_pids[i] == pid) {
            children_pids[i] = -1;
            return;
        }
    }
}

void __link(int index, int controller, int *children_pids) {
    int device_pid = get_device_pid(index, children_pids);

    if (device_pid == -1) {
        cprintf("Errore! Non esiste questo dispositivo.\n");
        return;
    }

    //int controller_pid = get_controller_pid();
}