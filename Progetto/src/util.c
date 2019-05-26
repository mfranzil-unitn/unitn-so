#include "util.h"

void lprintf(const char *__restrict__ __format, ...) {
    va_list(args);
    /*  printf("%d - ", time(NULL)); */
    va_start(args, __format);
    printf("\033[0;31m");
    vfprintf(stdout, __format, args);
    printf("\033[0m");
    fflush(stdout);
}

int parse(char buf[][MAX_BUF_SIZE], int cmd_n) {
    char ch;  /* carattere usato per la lettura dei comandi */
    int ch_i; /* indice del carattere corrente */

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
    /* Divide una stringa presa dalla pipe */
    /* a seconda del dispositivo. */

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

char **split_fixed(char *__buf, int __count) {
    char *tokenizer = strtok(__buf, "|");
    char **vars = malloc((__count + 3) * sizeof(*vars));
    int j = 0;
    while (tokenizer != NULL && j <= __count) {
        vars[j++] = tokenizer;
        tokenizer = strtok(NULL, "|");
    }

    vars[j] = "\0";

    return vars;
}

char *get_shell_text() {
    char host[MAX_BUF_SIZE];
    char *user;
    user = (char *)malloc(MAX_BUF_SIZE * sizeof(char));
    gethostname(host, MAX_BUF_SIZE);
    cuserid(user);
    return strcat(strcat(user, "@"), host);
}

void get_pipe_name(int pid, char *pipe_str) {
    sprintf(pipe_str, "%s%d", PIPES_POSITIONS, pid);
}

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

int get_device_pid(int device_identifier, int *children_pids, char **raw_info) {
    int i, children_pid, possible_pid;
    char var_buffer[MAX_BUF_SIZE];
    char *tmp, **vars;
    char buf_info[MAX_BUF_SIZE];

    //printf("\n----------------\n\nEntering get_device_pid(%d);\n", device_identifier);

    for (i = 0; i < MAX_CHILDREN; i++) { /* l'indice i è logicamente indipendente dal nome/indice del dispositivo */
        children_pid = children_pids[i];
        if (children_pid != -1) {
            //printf("i: %d, children_pid: %d\n", i, children_pid);

            tmp = get_raw_device_info(children_pid);
            if (tmp != NULL) {
                /* Evito fastidiose modifiche a TMP da strtok */
                strcpy(var_buffer, tmp);

                if (strncmp(tmp, HUB_S, 1) == 0 || strncmp(tmp, TIMER_S, 1) == 0) {
                    char *second_raw_info = NULL;
                    possible_pid = hub_tree_pid_finder(var_buffer, device_identifier, &second_raw_info);

                    //printf("hub_tree_pid_finder(): I:%d, P:%d, %s\n", device_identifier, possible_pid, second_raw_info);

                    if (possible_pid != -1) {
                        *raw_info = second_raw_info;
                        return possible_pid;
                    }
                } else {
                    vars = split(var_buffer);
                    //printf("split(): I: %d, P: %d, %s", device_identifier, children_pid, var_buffer);
                    if (vars != NULL && atoi(vars[2]) == device_identifier) {
                        *raw_info = tmp;
                        free(vars);
                        return children_pid;
                    }
                }
            } else {
                //printf("TMP IS FCKN NULLL\n");
            }
        }
    }
    return -1;
}

/* DEPRECATED
char **get_device_info(int pid) {
    if (kill(pid, SIGUSR1) != 0) {
        return NULL;
    }
    char pipe_str[MAX_BUF_SIZE];
    char tmp[MAX_BUF_SIZE];

    get_pipe_name(pid, pipe_str);

    int fd = open(pipe_str, O_RDONLY);

    if (fd > 0) {
        read(fd, tmp, MAX_BUF_SIZE);
        char **vars = split(tmp);
        close(fd);
        return vars;
    }
    return NULL;
}*/

char *get_raw_device_info(int pid) {
    /* const int MAX_ATTEMPTS = 3; */
    /*  lprintf("DEBUG: Attempt 0"); */

    /*  int i; */
    /*   for (i = 0; i < MAX_ATTEMPTS; i++) { */
    /*    lprintf("\b%d", i + 1); */

    int enter = 0;
    key_t key;
    int msgid;
    int ret;
    char *tmp;

    char pipe_str[MAX_BUF_SIZE];
    int fd, _read;
    fd_set set;
    struct timeval timeout;

    int kill_o;
    /*printf("GET_RAW_DEVICE: %d\n", pid);
    fflush(stdout);*/

    kill(pid, SIGUSR1);
    key = ftok("/tmp/ipc/mqueues", pid);
    msgid = msgget(key, 0666 | IPC_CREAT);
    /*printf("KEY: %d PID: %d MESSAGE ID: %d MESSAGE TYPE: %d\n",key, pid, msgid,  message.mesg_type); */

    ret = msgrcv(msgid, &message, sizeof(message), 1, 0);
    /*printf("HERE RET: %d\n", ret); */
    /*printf("Message: %s\n", message.mesg_text); */
    if (ret != -1) {
        tmp = malloc(MAX_BUF_SIZE * sizeof(char));
        sprintf(tmp, "%s", message.mesg_text);
        //printf("TMP: %s\n", tmp);
        return tmp;
    } else {
        return NULL;
    }
    if (enter) {
        kill_o = kill(pid, SIGUSR1);
        if (kill_o != 0) {
            return NULL; /*continue; */
        }

        tmp = malloc(MAX_BUF_SIZE * sizeof(tmp));
        get_pipe_name(pid, pipe_str);
        //printf("%s\n", pipe_str);
        fd = open(pipe_str, O_RDONLY);
        if (!(fd > 0)) {
            //(printf("FD_ERROR: %d", fd);
        } else {
           // printf("FD: %d\n", fd);
        }

        /* Initialize the file descriptor set. */
        FD_ZERO(&set);
        FD_SET(fd, &set);

        /* Initialize the timeout data structure. */
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;

        /* select returns 0 if timeout, 1 if input available, -1 if error. */
        if (fd > 0 && select(FD_SETSIZE, &set, NULL, NULL, &timeout) == 1) {
            //printf("DEBUG: In read for PID: %d and pipe %s\n", pid, pipe_str);
            _read = read(fd, tmp, MAX_BUF_SIZE);
            /*lprintf("End read, TMP: %s\n", tmp);*/
            /* Pulizia */
            close(fd);
            if (_read != 0) {
                /*  lprintf("\n"); */
                /*printf("TMP: %s\n", tmp); */
                return tmp;
            } else {
                /* lprintf("Errore durante la read.\n");*/
            }
        } else {
            if (fd < 0) {
               // printf("ERRORE FD\n");
            } else {
               // printf("Timed out\n");
            }
            close(fd);
            return NULL; /*continue; */
        }
        /*} */
        /* lprintf("\n"); */
        return NULL;
    }
}

int is_controller(int pid, char *raw_info) {
    char **vars = split(raw_info);
    int id = atoi(vars[0]);
    free(vars);
    return id == HUB || id == TIMER;
}

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

void hub_tree_print(char **vars) {
    char device_name[MAX_BUF_SIZE];

    if (strcmp(vars[0], HUB_S) == 0) {
       printf("Hub (PID %s, Indice %s) Stato: %s Dispositivi collegati: %s %s",
               vars[1], vars[2], (atoi(vars[3]) == 1 || atoi(vars[3]) == 2) ? "Acceso" : "Spento", vars[4], ( atoi(vars[3]) == 2 || atoi(vars[3]) == 3 ) ? " -> Override" : "");
    } else if (strcmp(vars[0], TIMER_S) == 0) {
        printf("Timer (PID: %s, Indice: %s), Stato: %s, Orari: %s%s:%s%s -> %s%s:%s%s, Collegati: %s %s",
               vars[1], vars[2],
               (atoi(vars[3]) == 1 || atoi(vars[3]) == 2) ? "Acceso" : "Spento",
               atoi(vars[4]) < 10 ? "0" : "", vars[4],
               atoi(vars[5]) < 10 ? "0" : "", vars[5],               
               atoi(vars[6]) < 10 ? "0" : "", vars[6],               
               atoi(vars[7]) < 10 ? "0" : "", vars[7],
               vars[8],
               (atoi(vars[3]) == 2 || atoi(vars[3]) == 3 ) ? " -> Override" : "");
    } else {
        get_device_name(atoi(vars[0]), device_name);
        device_name[0] += 'A' - 'a';

        printf("%s (PID %s, Indice %s)", device_name, vars[1], vars[2]);
    }
}

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

void hub_tree_parser(char *__buf) {
    char *tokenizer = strtok(__buf, "|");
    char *old = NULL;
    int level = 0;
    char **vars;
    int i = 0;
    int to_be_printed = 1;

    /* printf("Level_a %d\n", level); */

    vars = malloc((FRIDGE_PARAMETERS + 4) * sizeof(*vars));

    while (tokenizer != NULL) {
        /* printf("\nLevel %d tokenizer %s to_be_printed %d\n", level, tokenizer, to_be_printed); */
        if (strcmp(tokenizer, "<!") == 0) {
            to_be_printed += atoi(old) - 1;
            hub_tree_spaces(level);
            i = 0;
            ++level; /*  printf("\nLevel_b %d\n", level); */
            hub_tree_print(vars);
        } else if (strcmp(tokenizer, "!>") == 0) {
            --level; /*  printf("\nLevel_c %d\n", level); */
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
            /*printf("%s, ", tokenizer); */
        }
        old = tokenizer;
        tokenizer = strtok(NULL, "|");
    }
    printf("\n");
    free(vars);
}

int hub_tree_pid_finder(char *__buf, int id, char **raw_info) {
    char *tokenizer = strtok(__buf, "|");
    char *old = NULL;
    char **vars;
    int i = 0, j;
    int children = 1;
    int level = 0;
    int pid_to_be_returned = -1;
    /*printf("Level %d\n", level); */

    /* DISPOSITIVO; PID; ID */

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

            if (strcmp(old, "!>") != 0 && children > 0) {
                // CASO IN CUI E' FINITO UN FIGLIO SOLO
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
            /*printf("%s, ", tokenizer); */
        }
        old = tokenizer;
        tokenizer = strtok(NULL, "|");
    }
    free(vars);
    return -1;
}

int get_shell_pid() {
    /*Creo message queue per comunicare shellpid */
    int msgid_sh, shellpid;
    key_t key_sh;

    key_sh = ftok("/tmp/ipc", 2000);
    msgid_sh = msgget(key_sh, 0666 | IPC_CREAT);
    message.mesg_type = 1;
    msgrcv(msgid_sh, &message, sizeof(message), 1, IPC_NOWAIT);
    shellpid = atoi(message.mesg_text);
    sprintf(message.mesg_text, "%d", shellpid);
    msgsnd(msgid_sh, &message, MAX_BUF_SIZE, 1);
    /*printf("SHELLPID FUORI\n"); */
    return shellpid;
}

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