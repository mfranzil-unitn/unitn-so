#include "util.h"

int print_mode = 1;

void cprintf(const char *__restrict__ __format, ...) {
    if (print_mode) {
        va_list(args);
        //  cprintf("%d - ", time(NULL));
        va_start(args, __format);
        vprintf(__format, args);
    }
}

int parse(char buf[][MAX_BUF_SIZE], int cmd_n) {
    char ch;   // carattere usato per la lettura dei comandi
    int ch_i;  // indice del carattere corrente

    ch = ' ';
    ch_i = -1;
    cmd_n = 0;
    buf[cmd_n][0] = '\0';
    while (ch != EOF && ch != '\n') {
        ch = getchar();
        if (ch == ' ') {
            buf[cmd_n++][++ch_i] = '\0';
            ch_i = -1;
        } else {
            buf[cmd_n][++ch_i] = ch;
        }
    }
    buf[cmd_n][ch_i] = '\0';
    buf[cmd_n + 1][0] = '\0';

    return cmd_n;
}

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
        case HUB:
            __count = HUB_PARAMETERS;
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

    while (tokenizer != NULL && j <= __count) {
        vars[j++] = tokenizer;
        tokenizer = strtok(NULL, "|");
    }

    return vars;
}

char *get_shell_text() {
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

char *get_pipe_name(int pid) {
    char *pipe_str = malloc(4 * sizeof(char));
    sprintf(pipe_str, "%s%i", PIPES_POSITIONS, pid);
    return pipe_str;
}

void get_device_name(int device_type, char *buf) {
    switch (device_type) {
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
        case HUB:
            sprintf(buf, "hub");
            break;
        default:
            sprintf(buf, "-");
            break;
    }
}

void get_device_name_str(char *device_type, char *buf) {
    if (strcmp(device_type, "bulb") == 0) {
        sprintf(buf, "lampadina");
    } else if (strcmp(device_type, "fridge") == 0) {
        sprintf(buf, "frigo");
    } else if (strcmp(device_type, "window") == 0) {
        sprintf(buf, "finestra");
    } else if (strcmp(device_type, "controller") == 0) {
        sprintf(buf, "centralina");
    } else if (strcmp(device_type, "hub") == 0) {
        sprintf(buf, "hub");
    } else {
        sprintf(buf, "-");
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
        pipe_str = get_pipe_name(children_pid);
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
        cprintf("Errore! Non esiste questo dispositivo.\n");
        return;
    }

    char *pipe_str = get_pipe_name(pid);  // Nome della pipe
    char tmp[MAX_BUF_SIZE];               // dove ci piazzo l'output della pipe
    char **vars = NULL;                   // output della pipe, opportunamente diviso
    char pipe_message[MAX_BUF_SIZE];      // buffer per la pipe

    if (kill(pid, SIGUSR1) != 0) {
        // apertura della pipe fallita
        cprintf("Errore! Impossibile notificare il dispositivo. Errno: %i\n", errno);
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
                cprintf("Lampadina accesa.\n");
            } else if (strcmp(buf[3], "off") == 0 && status == 1) {
                write(fd, pipe_message, MAX_BUF_SIZE);
                kill(pid, SIGUSR2);
                cprintf("Lampadina spenta.\n");
            } else if (strcmp(buf[3], "off") == 0 && status == 0) {  // Spengo una lampadina spenta
                cprintf("Stai provando a spegnere una lampadina spenta!\n");
            } else if (strcmp(buf[3], "on") == 0 && status == 1) {  // Spengo una lampadina accesa
                cprintf("Stai provando a accendere una lampadina accesa!\n");
            } else {
                cprintf("Sintassi non corretta. Sintassi: switch <bulb> accensione <on/off>\n");
            }
        } else {
            cprintf("Operazione non permessa su una lampadina!\nOperazioni permesse: accensione\n");
        }
    } else if (strncmp(tmp, "2", 1) == 0) {  // Fridge
        if (strcmp(buf[2], "apertura") == 0) {
            vars = split(tmp);  // parametri: tipo, pid, indice, stato, tempo di accensione
            int status = atoi(vars[3]);
            sprintf(pipe_message, "0|0");

            if (strcmp(buf[3], "on") == 0 && status == 0) {
                write(fd, pipe_message, MAX_BUF_SIZE);
                kill(pid, SIGUSR2);
                cprintf("Frigorifero aperto.\n");
            } else if (strcmp(buf[3], "off") == 0 && status == 1) {
                write(fd, pipe_message, MAX_BUF_SIZE);
                kill(pid, SIGUSR2);
                cprintf("Frigorifero chiuso.\n");
            } else if (strcmp(buf[3], "off") == 0 && status == 0) {  // Chiudo frigo già chiuso
                cprintf("Stai provando a chiudere un frigorifero già chiuso.\n");
            } else if (strcmp(buf[3], "on") == 0 && status == 1) {  // Apro frigo già aperto
                cprintf("Stai provando a aprire un frigorifero già aperto.\n");
            } else {
                cprintf("Sintassi non corretta. Sintassi: switch <fridge> apertura <on/off>\n");
            }
        } else if (strcmp(buf[2], "temperatura") == 0) {
            sprintf(pipe_message, "1|%s", buf[3]);

            write(fd, pipe_message, MAX_BUF_SIZE);
            kill(pid, SIGUSR2);
            cprintf("Temperatura modificata con successo a %s°C.\n", buf[3]);
        } else {
            cprintf("Operazione non permessa su un frigorifero! Operazioni permesse: <temperatura/apertura>\n");
        }
    } else if (strncmp(tmp, "3", 1) == 0) {  // Window
        if (((strcmp(buf[2], "apertura") != 0) || (strcmp(buf[2], "apertura") == 0 && strcmp(buf[3], "off") == 0)) &&
            ((strcmp(buf[2], "chiusura") != 0) || (strcmp(buf[2], "chiusura") == 0 && strcmp(buf[3], "off") == 0))) {
            cprintf("Operazione non permessa: i pulsanti sono solo attivi!\n");
            // se off non permetto
            return;
        }

        vars = split(tmp);  // parametri: tipo, pid, indice, stato, tempo di accensione
        int status = atoi(vars[3]);
        sprintf(pipe_message, "0|0");

        if (strcmp(buf[2], "apertura") == 0 && status == 0) {
            write(fd, pipe_message, sizeof(pipe_message));
            kill(pid, SIGUSR2);
            cprintf("Finestra aperta.\n");
        } else if (strcmp(buf[2], "chiusura") == 0 && status == 1) {
            write(fd, pipe_message, sizeof(pipe_message));
            kill(pid, SIGUSR2);
            cprintf("Finestra chiusa.\n");
        } else {
            cprintf("Operazione non permessa: pulsante già premuto.\n");
        }
    } else {  // tutti gli altri dispositivi
        cprintf("Dispositivo non supportato.\n");
        //  return; Possibile errore di allocazione perché vars non è stato allocato?
    }
    close(fd);
    free(vars);
}

void __info(char buf[][MAX_BUF_SIZE], int *children_pids) {
    int pid = get_device_pid(atoi(buf[1]), children_pids);

    if (pid == -1) {
        cprintf("Errore! Non esiste questo dispositivo.\n");
        return;
    }

    char *pipe_str = get_pipe_name(pid);
    char **vars = NULL;
    char tmp[MAX_BUF_SIZE];  // dove ci piazzo l'output della pipe

    // apertura della pipe fallita
    if (kill(pid, SIGUSR1) != 0) {
        cprintf("Errore! Sistema: codice errore %i\n", errno);
        return;
    }

    int fd = open(pipe_str, O_RDONLY);
    read(fd, tmp, MAX_BUF_SIZE);
    close(fd);
    free(pipe_str);

    if (strncmp(tmp, "1", 1) == 0) {
        // Lampadina - parametri: tipo, pid, indice, stato, tempo di accensione
        vars = split(tmp);

        cprintf("Oggetto: Lampadina\nPID: %s\nIndice: %s\nStato: %s\nTempo di accensione: %s\n",
                vars[1], vars[2], atoi(vars[3]) ? "ON" : "OFF", vars[4]);
    } else if (strncmp(tmp, "2", 1) == 0) {
        // Frigo -  parametri: tipo, pid, indice, stato, tempo di apertura, delay
        // percentuale riempimento, temperatura interna
        vars = split(tmp);

        cprintf("Oggetto: Frigorifero\n");
        if (vars[8] != NULL) {  //&& vars[8] != "" && vars[8][0] != 0) {
            cprintf("[!!] Messaggio di log: <%s>\n", vars[8]);
        }

        cprintf("PID: %s\nIndice: %s\nStato: %s\nTempo di apertura: %s sec\n",
                vars[1], vars[2], atoi(vars[3]) ? "Aperto" : "Chiuso", vars[4]);
        cprintf("Delay richiusura: %s sec\nPercentuale riempimento: %s\nTemperatura: %s°C\n",
                vars[5], vars[6], vars[7]);
    } else if (strncmp(tmp, "3", 1) == 0) {
        // Finestra - parametri: tipo, pid, indice, stato, tempo di accensione
        vars = split(tmp);
        cprintf("Oggetto: Finestra\nPID: %s\nIndice: %s\nStato: %s\nTempo di apertura: %s sec\n",
                vars[1], vars[2], atoi(vars[3]) ? "Aperto" : "Chiuso", vars[4]);
    } else if (strncmp(tmp, "4", 1) == 0) {
        // Hub -  parametri: tipo, pid, stato indice
        vars = split(tmp);
        cprintf("Oggetto: Hub\nPID: %s\nIndice: %s\nStato: %s\n",
                vars[1], vars[2], atoi(vars[3]) ? "ON" : "OFF");
    } else {
        cprintf("Dispositivo non supportato.\n");
    }
    free(vars);
}