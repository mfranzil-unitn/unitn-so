#include "util.h"

/* funzione per il debuge */
void lprintf(const char *__restrict__ __format, ...) { 
    va_list(args);
    va_start(args, __format);
    printf("\033[0;31m");
    vfprintf(stdout, __format, args);
    printf("\033[0m");
    fflush(stdout);
}

/* prende comandi da tastiera  */
int parse(char buf[][MAX_BUF_SIZE], int cmd_n) { 
    char ch;  /* carattere usato per la lettura dei comandi */
    int ch_i; /* indice del carattere corrente */

    ch = ' ';
    ch_i = -1;
    cmd_n = 0;
    buf[cmd_n][0] = '\0';
    
    /* continua a leggere fino ad in nuova riga (INVIO) o EOF */
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

/* Divide una stringa presa dalla pipe a seconda del dispositivo. */
char **split(char *__buf) { 
    /* La prima cifra di __buf è sempre il tipo di dispositivo. */
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
        case TIMER:
            __count = TIMER_PARAMETERS;
            break;
        default:
            __count = 1;
            break;
    }

    return split_fixed(__buf, __count);
}

/* splitta la __buf in un array */
char **split_fixed(char *__buf, int __count) { 
    char *tokenizer = strtok(__buf, "|");
    char **vars = malloc((__count + 3) * sizeof(*vars));
    int j = 0;
    while (tokenizer != NULL && j <= __count) {
        vars[j++] = tokenizer;
        tokenizer = strtok(NULL, "|");
    }
    return vars;
}

/* scrive il intestazione della shell */
char *get_shell_text() { 
    char host[MAX_BUF_SIZE];
    char *user;
    user = (char *)malloc(MAX_BUF_SIZE * sizeof(char));
    gethostname(host, MAX_BUF_SIZE);
    cuserid(user);
    return strcat(strcat(user, "@"), host);
}

/* dal pid restituisce la pipe */
void get_pipe_name(int pid, char *pipe_str) { 
    sprintf(pipe_str, "%s%d", PIPES_POSITIONS, pid);
}

/* ritorna il tipo di device */
void get_device_name(int device_type, char *buf) { 
    switch (device_type) {
        case BULB:
            sprintf(buf, BULB_IT);
            break;
        case FRIDGE:
            sprintf(buf, FRIDGE_IT);
            break;
        case WINDOW:
            sprintf(buf, WINDOW_IT);
            break;
        case CONTROLLER:
            sprintf(buf, CONTROLLER_IT);
            break;
        case HUB:
            sprintf(buf, HUB_IT);
            break;
        case TIMER:
            sprintf(buf, TIMER_IT);
            break;
        default:
            sprintf(buf, INVALID_OUTPUT);
            break;
    }
}

/* ritorna il nome del device */
void get_device_name_str(char *device_type, char *buf) { 
    if (strcmp(device_type, BULB_ENG) == 0) {
        sprintf(buf, BULB_IT);
    } else if (strcmp(device_type, FRIDGE_ENG) == 0) {
        sprintf(buf, FRIDGE_IT);
    } else if (strcmp(device_type, WINDOW_ENG) == 0) {
        sprintf(buf, WINDOW_IT);
    } else if (strcmp(device_type, CONTROLLER_ENG) == 0) {
        sprintf(buf, CONTROLLER_IT);
    } else if (strcmp(device_type, HUB_ENG) == 0) {
        sprintf(buf, HUB_IT);
    } else if (strcmp(device_type, TIMER_ENG) == 0) {
        sprintf(buf, TIMER_IT);
    } else {
        sprintf(buf, INVALID_OUTPUT);
    }
}

/* in input ha l'identificativo del device e riempie raw_info(array di stringhe) con le sue informazioni */
int get_device_pid(int device_identifier, int *children_pids, char **raw_info) {
    int i, children_pid, possible_pid;
    char var_buffer[MAX_BUF_SIZE];
    char *tmp, **vars;

    /* l'indice i è logicamente indipendente dal nome/indice del dispositivo */
    for (i = 0; i < MAX_CHILDREN; i++) { 
        children_pid = children_pids[i];
        if (children_pid != -1) {
            tmp = get_raw_device_info(children_pid);
            if (tmp != NULL) {
                strcpy(var_buffer, tmp);

                if (strncmp(tmp, HUB_S, 1) == 0 || strncmp(tmp, TIMER_S, 1) == 0) {
                    char *second_raw_info = NULL;
                    possible_pid = hub_tree_pid_finder(var_buffer, device_identifier, &second_raw_info);

                    if (possible_pid != -1) {
                        *raw_info = second_raw_info;
                        return possible_pid;
                    }
                } else {
                    vars = split(var_buffer);
                    if (vars != NULL && atoi(vars[2]) == device_identifier) {
                        *raw_info = tmp;
                        free(vars);
                        return children_pid;
                    }
                }
            } else {
                printf("Errore tmp(null)\n");
            }
        }
    }
    return -1;
}

/* dal pid ritorna le informazioni del device */
char *get_raw_device_info(int pid) { 
    key_t key;
    int msgid;
    int ret;
    char *tmp;
    
    kill(pid, SIGUSR1);
    key = ftok("/tmp/ipc/mqueues", pid);
    msgid = msgget(key, 0666 | IPC_CREAT);
    
    ret = msgrcv(msgid, &message, sizeof(message), 1, 0);
    
    if (ret != -1) {
        tmp = malloc(MAX_BUF_SIZE * sizeof(char));
        sprintf(tmp, "%s", message.mesg_text);
        return tmp;
    } else {
        return NULL;
    }
}

 /* controlla se il device è un controller */
int is_controller(int pid, char *raw_info) {
    char **vars = split(raw_info);
    int id = atoi(vars[0]);
    free(vars);
    return id == HUB || id == TIMER;
}

/* controlla se il controller ha ancora spazio */
int controller_is_full(int pid, char *raw_info) { 
    char **vars;
    int count;

    vars = split(raw_info);
    if (atoi(vars[0]) == HUB) {
        count = atoi(vars[4]);
        return count >= MAX_CHILDREN;
    } else if (atoi(vars[0]) == TIMER) {
        count = atoi(vars[8]);
        return count != 0;
    } else {
        return 1;
    }
    free(vars);
}

/* stampa l'albero */
void hub_tree_print(char **vars) { 
    char device_name[MAX_BUF_SIZE];

    if (strcmp(vars[0], HUB_S) == 0) { /* controlla se è un hub */
       printf("Hub (PID %s, Indice %s) Stato: %s Dispositivi collegati: %s %s",
               vars[1], vars[2], (atoi(vars[3]) == 1 || atoi(vars[3]) == 2) ? "Acceso" : "Spento", vars[4], ( atoi(vars[3]) == 2 || atoi(vars[3]) == 3 ) ? " -> Override" : "");
    } else if (strcmp(vars[0], TIMER_S) == 0) { /* controlla se è un timer */
        printf("Timer (PID: %s, Indice: %s), Stato: %s, Orari: %s%s:%s%s -> %s%s:%s%s, Collegati: %s %s",
               vars[1], vars[2],
               (atoi(vars[3]) == 1 || atoi(vars[3]) == 2) ? "Acceso" : "Spento",
               atoi(vars[4]) < 10 ? "0" : "", vars[4],
               atoi(vars[5]) < 10 ? "0" : "", vars[5],               
               atoi(vars[6]) < 10 ? "0" : "", vars[6],               
               atoi(vars[7]) < 10 ? "0" : "", vars[7],
               vars[8],
               (atoi(vars[3]) == 2 || atoi(vars[3]) == 3 ) ? " -> Override" : "");
    } else { /* altrimenti è un device foglia */
        get_device_name(atoi(vars[0]), device_name);
        device_name[0] += 'A' - 'a';

        printf("%s (PID %s, Indice %s)", device_name, vars[1], vars[2]);
    }
}

/* stampa l'indentazione a livelli dell'albero */
void hub_tree_spaces(int level) { 
    int j;

    if (level > 0) {
        printf("\n");
        for (j = 0; j < level; j++) {
            printf("  ");
        }
        printf("∟ ");
    }
}

/*
a seguente funzione prende in input una stringa delle informazioni del dispositivo e ne stampa i contenuti in maniera iterativa, seguendo il protocollo specificato nel readme. Se riconosciuta la fine di un dispositivo, la variabile vars (Array di stringhe) viene svuotata e il contenuto stampato con le due funzioni ausiliarie
*/
void hub_tree_parser(char *__buf) {
    char *tokenizer = strtok(__buf, "|");
    char *old = NULL;
    int level = 0; /* profondità dell'albero */
    char **vars;
    int i = 0;
    int to_be_printed = 1; /* numero di oggetti che devono essere ancora stampati */

    /* 
     * <! = inizio figli
     * !  = altro figlio
     * !> = fine figli
     */

    vars = malloc((FRIDGE_PARAMETERS + 4) * sizeof(*vars));

    while (tokenizer != NULL) {
        if (strcmp(tokenizer, "<!") == 0) {
            to_be_printed += atoi(old) - 1;
            hub_tree_spaces(level);
            i = 0;
            ++level;
            hub_tree_print(vars);
        } else if (strcmp(tokenizer, "!>") == 0) {
            --level;
            if (strcmp(old, "<!") != 0 && strcmp(old, "!") != 0 && to_be_printed > 0) {
                i = 0;
                hub_tree_spaces(level);
                hub_tree_print(vars);
                to_be_printed--;
            }
        } else if (strcmp(tokenizer, "!") == 0) {
            if (strcmp(old, "!>") != 0 && to_be_printed > 0) {
                i = 0;
                hub_tree_spaces(level);
                hub_tree_print(vars);
                to_be_printed--;
            }
        } else {
            vars[i++] = tokenizer;
        }
        old = tokenizer;
        tokenizer = strtok(NULL, "|");
    }
    printf("\n");
    free(vars);
}

/* individua la sottostringa di un dato dispositivo, dato l'indice e la assegna al riferimento dato. Poi restituisce il l'id */
int hub_tree_pid_finder(char *__buf, int id, char **raw_info) {
    char *tokenizer = strtok(__buf, "|");
    char *old = NULL;
    char **vars;
    int i = 0, j;
    int children = 1;
    int level = 0;
    int pid_to_be_returned = -1;
    int found_flag = -1;
    char *target;
    vars = malloc((FRIDGE_PARAMETERS + 4) * sizeof(*vars));

    while (tokenizer != NULL) {
        if (strcmp(tokenizer, "<!") == 0) {
            if (found_flag >= 0) {
                target += sprintf(target, "|<!");
            }
            children += atoi(old) - 1;
            level++;
            if (atoi(vars[2]) == id) {
                found_flag = level - 1;

                *raw_info = malloc(MAX_BUF_SIZE * sizeof(raw_info));
                target = *raw_info;

                pid_to_be_returned = atoi(vars[1]);
                target += sprintf(target, "%s", vars[0]);
                for (j = 1; j < i; j++) {
                    target += sprintf(target, "|%s", vars[j]);
                }
                target += sprintf(target, "|<!");
            }
            i = 0;
        } else if (strcmp(tokenizer, "!>") == 0) {
            level--;

            if (found_flag >= 0) {
                target += sprintf(target, "|!>");
            }

            if (found_flag == level) {
                return pid_to_be_returned;
            }

            if (strcmp(old, "<!") != 0 && strcmp(old, "!") != 0 && children > 0) {
                i = 0;
                if (atoi(vars[2]) == id) {
                    return atoi(vars[1]);
                }
                children--;
            }
        } else if (strcmp(tokenizer, "!") == 0) {
            if (found_flag >= 0) {
                target += sprintf(target, "|!");
            }

            if (strcmp(old, "!>") != 0 && children > 0) { /* Caso di un figlio solo */
                if (atoi(vars[2]) == id) {
                    *raw_info = malloc(MAX_BUF_SIZE * sizeof(raw_info));
                    target = *raw_info;

                    target += sprintf(target, "%s", vars[0]);
                    for (j = 1; j < i; j++) {
                        target += sprintf(target, "|%s", vars[j]);
                    }
                    return atoi(vars[1]);
                }
                i = 0;

                children--;
            }
        } else {
            vars[i++] = tokenizer;
            if (found_flag >= 0) {
                target += sprintf(target, "|%s", tokenizer);
            }

        }
        old = tokenizer;
        tokenizer = strtok(NULL, "|");
    }
    free(vars);
    return -1;
}

/* ritorna il pid della shell */
int get_shell_pid() { 
    /*Creo message queue per comunicare shellpid */
    int msgid_sh, shellpid, ret;
    key_t key_sh;

    key_sh = ftok("/tmp", 2000);
    msgid_sh = msgget(key_sh, 0666 | IPC_CREAT);
   
    ret = msgrcv(msgid_sh, &message, sizeof(message), 1, IPC_NOWAIT);
    if (ret != -1){
    shellpid = atoi(message.mesg_text);
    message.mesg_type = 1;
    sprintf(message.mesg_text, "%d", shellpid);
    msgsnd(msgid_sh, &message, MAX_BUF_SIZE, 1);
    }
    else{
        shellpid = -1;
    }
    return shellpid;
}

/* divide i figli dei un hub */
char **split_sons(char *__buf, int __count) { 
    int j = 0;
    char *tokenizer;
    char **vars;

    tokenizer = strtok(__buf, "-");
    vars = malloc((__count + 3) * sizeof(*vars));
    while (tokenizer != NULL && j <= __count) {
        vars[j++] = tokenizer;
        tokenizer = strtok(NULL, "-");
    }
    vars[j] = "\0";
    return vars;
}
