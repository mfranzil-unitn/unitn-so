#include "util.h"

char **split(char *__buf) {
    // Divide una stringa presa dalla pipe
    // a seconda del dispositivo.

    // La prima cifra di __buf è sempre il tipo di dispositivo.

    int device = __buf[0] - '0';
    int __count;

    switch (device) {
        case BULB:
            __count = BULB_PARAMETERS;
            break;
        case FRIDGE:
            __count = FRIDGE_PARAMETERS;
            break;
        case WINDOW:
            __count = WINDOW_PARAMETERS;
            break;
        default:
            __count = 1;
            break;
    }

    char *tokenizer = strtok(__buf, "|");
    char **vars = malloc(__count * sizeof(char *));
    int j = 0;

    while (tokenizer != NULL && j < __count) {
        vars[j++] = tokenizer;
        tokenizer = strtok(NULL, "|");
    }

    return vars;
}

char **split_fixed(char *__buf, int __count) {
    char *tokenizer = strtok(__buf, "|");
    char **vars = malloc(__count * sizeof(char *));
    int j = 0;

    while (tokenizer != NULL && j < __count) {
        vars[j++] = tokenizer;
        tokenizer = strtok(NULL, "|");
    }

    return vars;
}

char *getUserName() {
    int uid = geteuid();
    struct passwd *pw = getpwuid(uid);
    if (pw) {
        char hostname[MAX_BUF_SIZE];
        hostname[MAX_BUF_SIZE - 1] = '\0';
        gethostname(hostname, MAX_BUF_SIZE - 1);
        return strcat(strcat(pw->pw_name, "@"), hostname);
    }
    return "host";
}

char *pipename(int pid) {
    char *pipe_str = malloc(4 * sizeof(char));
    sprintf(pipe_str, "%s%i", PIPES_POSITIONS, pid);
    return pipe_str;
}

void get_device_name(int device_type, char* buf) {
    switch(device_type) {
        case BULB:
            sprintf(buf, "lampadina");
            break;
        case FRIDGE:
            sprintf(buf, "frigo");
            break;
        case WINDOW:
            sprintf(buf, "finestra");
            break;
        case CONTROLLER:
            sprintf(buf, "centralina");
            break;
        default:
            sprintf(buf, "-");
            break;
    }
}

int get_device_pid(int device_identifier, int *children_pids) {
    // prende come input l'indice/nome del dispositivo, ritorna il PID
    char *pipe_str = NULL;
    int res = -1;

    int i;
    for (i = 0; i < MAX_CHILDREN; i++) {  // l'indice i è logicamente indipendente dal nome/indice del dispositivo
        int children_pid = children_pids[i];
        char tmp[MAX_BUF_SIZE];

        if (children_pid == -1) {
            continue;  // dispositivo non più nei figli
        }

        kill(children_pid, SIGUSR1);
        pipe_str = pipename(children_pid);
        int fd = open(pipe_str, O_RDONLY);

        if (fd > 0) {
            read(fd, tmp, MAX_BUF_SIZE);
            char **vars = split(tmp);
            int tmp_int = atoi(vars[2]);
            // Pulizia
            free(vars);
            free(pipe_str);
            close(fd);

            if (tmp_int == device_identifier) {
                return children_pid;
            }
        }
    }
    return res;
}

void __switch(char buf[][MAX_BUF_SIZE], int *children_pids) {
    int pid = get_device_pid(atoi(buf[1]), children_pids);

    if (pid == -1) {
        printf("Errore! Non esiste questo dispositivo.\n");
        return;
    }

    char *pipe_str = pipename(pid);   // Nome della pipe
    char tmp[MAX_BUF_SIZE];           // dove ci piazzo l'output della pipe
    char **vars = NULL;               // output della pipe, opportunamente diviso
    char pipe_message[MAX_BUF_SIZE];  // buffer per la pipe

    if (kill(pid, SIGUSR1) != 0) {
        // apertura della pipe fallita
        printf("Errore! Impossibile notificare il dispositivo. Errno: %i\n", errno);
        return;
    }

    int fd = open(pipe_str, O_RDWR);
    read(fd, tmp, MAX_BUF_SIZE);
    free(pipe_str);

    if (strncmp(tmp, "1", 1) == 0) {  // Lampadina
        if (strcmp(buf[2], "accensione") == 0) {
            vars = split(tmp);  // parametri: tipo, pid, indice, stato, tempo di accensione,
            int status = atoi(vars[3]);
            sprintf(pipe_message, "0|0");

            if (strcmp(buf[3], "on") == 0 && status == 0) {
                write(fd, pipe_message, MAX_BUF_SIZE);
                kill(pid, SIGUSR2);
                printf("Lampadina accesa.\n");
            } else if (strcmp(buf[3], "off") == 0 && status == 1) {
                write(fd, pipe_message, MAX_BUF_SIZE);
                kill(pid, SIGUSR2);
                printf("Lampadina spenta.\n");
            } else if (strcmp(buf[3], "off") == 0 && status == 0) {  // Spengo una lampadina spenta
                printf("Stai provando a spegnere una lampadina spenta!\n");
            } else if (strcmp(buf[3], "on") == 0 && status == 1) {  // Spengo una lampadina accesa
                printf("Stai provando a accendere una lampadina accesa!\n");
            } else {
                printf("Sintassi non corretta. Sintassi: switch <bulb> accensione <on/off>\n");
            }
        } else {
            printf("Operazione non permessa su una lampadina!\nOperazioni permesse: accensione\n");
        }
    } else if (strncmp(tmp, "2", 1) == 0) {  // Fridge
        if (strcmp(buf[2], "apertura") == 0) {
            vars = split(tmp);  // parametri: tipo, pid, indice, stato, tempo di accensione
            int status = atoi(vars[3]);
            sprintf(pipe_message, "0|0");

            if (strcmp(buf[3], "on") == 0 && status == 0) {
                write(fd, pipe_message, MAX_BUF_SIZE);
                kill(pid, SIGUSR2);
                printf("Frigorifero aperto.\n");
            } else if (strcmp(buf[3], "off") == 0 && status == 1) {
                write(fd, pipe_message, MAX_BUF_SIZE);
                kill(pid, SIGUSR2);
                printf("Frigorifero chiuso.\n");
            } else if (strcmp(buf[3], "off") == 0 && status == 0) {  // Chiudo frigo già chiuso
                printf("Stai provando a chiudere un frigorifero già chiuso.\n");
            } else if (strcmp(buf[3], "on") == 0 && status == 1) {  // Apro frigo già aperto
                printf("Stai provando a aprire un frigorifero già aperto.\n");
            } else {
                printf("Sintassi non corretta. Sintassi: switch <fridge> apertura <on/off>\n");
            }
        } else if (strcmp(buf[2], "temperatura") == 0) {
            sprintf(pipe_message, "1|%s", buf[3]);

            write(fd, pipe_message, MAX_BUF_SIZE);
            kill(pid, SIGUSR2);
            printf("Temperatura modificata con successo a %s°C.\n", buf[3]);
        } else {
            printf("Operazione non permessa su un frigorifero! Operazioni permesse: <temperatura/apertura>\n");
        }
    } else if (strncmp(tmp, "3", 1) == 0) {  // Window
        if (((strcmp(buf[2], "apertura") != 0) || (strcmp(buf[2], "apertura") == 0 && strcmp(buf[3], "off") == 0)) &&
            ((strcmp(buf[2], "chiusura") != 0) || (strcmp(buf[2], "chiusura") == 0 && strcmp(buf[3], "off") == 0))) {
            printf("Operazione non permessa: i pulsanti sono solo attivi!\n");
            // se off non permetto
            return;
        }

        vars = split(tmp);  // parametri: tipo, pid, indice, stato, tempo di accensione
        int status = atoi(vars[3]);
        sprintf(pipe_message, "0|0");

        if (strcmp(buf[2], "apertura") == 0 && status == 0) {
            write(fd, pipe_message, sizeof(pipe_message));
            kill(pid, SIGUSR2);
            printf("Finestra aperta.\n");
        } else if (strcmp(buf[2], "chiusura") == 0 && status == 1) {
            write(fd, pipe_message, sizeof(pipe_message));
            kill(pid, SIGUSR2);
            printf("Finestra chiusa.\n");
        } else {
            printf("Operazione non permessa: pulsante già premuto.\n");
        }
    } else {  // tutti gli altri dispositivi
        printf("Dispositivo non supportato.\n");
        //  return; Possibile errore di allocazione perché vars non è stato allocato?
    }
    close(fd);
    free(vars);
}

void __info(char buf[][MAX_BUF_SIZE], int *children_pids) {
    int pid = get_device_pid(atoi(buf[1]), children_pids);

    if (pid == -1) {
        printf("Errore! Non esiste questo dispositivo.\n");
        return;
    }

    char *pipe_str = pipename(pid);
    char **vars = NULL;
    char tmp[MAX_BUF_SIZE];  // dove ci piazzo l'output della pipe

    // apertura della pipe fallita
    if (kill(pid, SIGUSR1) != 0) {
        printf("Errore! Sistema: codice errore %i\n", errno);
        return;
    }

    int fd = open(pipe_str, O_RDONLY);
    read(fd, tmp, MAX_BUF_SIZE);
    close(fd);
    free(pipe_str);

    if (strncmp(tmp, "1", 1) == 0) {  // Lampadina
        vars = split(tmp);
        // parametri: tipo, pid, stato, tempo di accensione, indice

        printf("Oggetto: Lampadina\nPID: %s\nIndice: %s\nStato: %s\nTempo di accensione: %s\n",
               vars[1], vars[2], atoi(vars[3]) ? "ON" : "OFF", vars[4]);
    } else if (strncmp(tmp, "2", 1) == 0) {  // Frigo
        vars = split(tmp);
        // parametri: tipo, pid, stato, tempo di apertura, indice, delay
        // percentuale riempimento, temperatura interna

        printf("Oggetto: Frigorifero\n");

        if (vars[8] != NULL && vars[8] != "" && vars[8][0] != 0) {
            printf("[!!] Messaggio di log: <%s>\n", vars[8]);
        }

        printf("PID: %s\nIndice: %s\nStato: %s\nTempo di apertura: %s sec\n",
               vars[1], vars[2], atoi(vars[3]) ? "Aperto" : "Chiuso", vars[4]);
        printf("Delay richiusura: %s sec\nPercentuale riempimento: %s\nTemperatura: %s°C\n",
               vars[5], vars[6], vars[7]);
    } else if (strncmp(tmp, "3", 1) == 0) {  // Finestra
        vars = split(tmp);
        // parametri: tipo, pid, stato, tempo di accensione, indice
        printf("Oggetto: Finestra\nPID: %s\nIndice: %s\nStato: %s\nTempo di apertura: %s sec\n",
               vars[1], vars[2], atoi(vars[3]) ? "Aperto" : "Chiuso", vars[4]);
    } else {
        printf("Dispositivo non supportato.\n");
    }
    free(vars);
}