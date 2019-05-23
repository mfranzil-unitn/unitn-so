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
        case TIMER:
            sprintf(buf, "timer");
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
    } else if (strcmp(device_type, "timer") == 0) {
        sprintf(buf, "timer");
    } else {
        sprintf(buf, "-");
    }
}

int get_device_pid(int device_identifier, int *children_pids, char **raw_info) {
    int i;
    char var_buffer[MAX_BUF_SIZE];

    for (i = 0; i < MAX_CHILDREN; i++) { /* l'indice i è logicamente indipendente dal nome/indice del dispositivo */
        int children_pid = children_pids[i];
        if (children_pid != -1) {
            //printf("GET_ DEVICE_PID: %d\n", children_pid);
            char *tmp = get_raw_device_info(children_pid);
            //printf("TMP: %s\n", tmp);
            if (tmp != NULL) {
                if (raw_info != NULL) {
                    *raw_info = tmp;
                }

                /* Evito fastidiose modifiche a TMP da strtok */
                strcpy(var_buffer, tmp);

                if (strncmp(tmp, HUB_S, 1) == 0) {
                    char buf_info[MAX_BUF_SIZE];
                    /* Evito fastidiose modifiche a TMP da strtok */
                    strcpy(buf_info, tmp);
                    //printf("BUF INFO %s\n", buf_info);
                    char **vars = split(buf_info);
                    if (vars != NULL && atoi(vars[2]) == device_identifier) {
                        //printf("SONO ENTRATO PER PID: %d\n", children_pid);
                        free(vars);
                        return children_pid;
                    }
                    int possible_pid = hub_tree_pid_finder(var_buffer, device_identifier);
                    //printf("Possible PID found: %d\n", possible_pid);
                    if (possible_pid != -1) {
                        return possible_pid;
                    }
                } else {
                    char **vars = split(var_buffer);
                    if (vars != NULL && atoi(vars[2]) == device_identifier) {
                        free(vars);
                        return children_pid;
                    }
                }
            } 
            else{
                printf("TMP IS FCKN NULLL\n");
            }
        }
    }
    if (raw_info != NULL) {
        *raw_info = NULL;
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

    int enter =0; 
    
    printf("GET_RAW_DEVICE: %d\n", pid);
    fflush(stdout);

    kill(pid, SIGUSR1);
    key_t key = ftok("/tmp/ipc/mqueues", pid);
    int msgid = msgget(key, 0666 | IPC_CREAT);
    //printf("KEY: %d PID: %d MESSAGE ID: %d MESSAGE TYPE: %d\n",key, pid, msgid,  message.mesg_type);

    char* tmp;
    int ret = msgrcv(msgid, &message, sizeof(message), 1, 0);
    //printf("HERE RET: %d\n", ret);
    //printf("Message: %s\n", message.mesg_text);
    if(ret!=-1){
            tmp = malloc(MAX_BUF_SIZE * sizeof(char));
            sprintf(tmp, "%s", message.mesg_text);
            printf("TMP: %s\n", tmp);
            return tmp;
    }
    else{
        return NULL;
    }





    if(enter){
    char pipe_str[MAX_BUF_SIZE];
    int fd, _read;
    char *tmp;
    fd_set set;
    struct timeval timeout;

    int kill_o = kill(pid, SIGUSR1);
    if (kill_o != 0) {
        return NULL; /*continue; */
    }

    tmp = malloc(MAX_BUF_SIZE * sizeof(tmp));
    get_pipe_name(pid, pipe_str);
    printf("%s\n", pipe_str);
    fd = open(pipe_str, O_RDONLY);
    if(!(fd > 0)){
        printf("FD_ERROR: %d", fd);
    }else{
        printf("FD: %d\n", fd);
    }

    /* Initialize the file descriptor set. */
    FD_ZERO(&set);
    FD_SET(fd, &set);

    /* Initialize the timeout data structure. */
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;

    /* select returns 0 if timeout, 1 if input available, -1 if error. */
    if (fd > 0 && select(FD_SETSIZE, &set, NULL, NULL, &timeout)==1) {
        lprintf("DEBUG: In read for PID: %d and pipe %s\n", pid, pipe_str);
        _read = read(fd, tmp, MAX_BUF_SIZE);
        lprintf("End read, TMP: %s\n", tmp);
        /* Pulizia */
        close(fd);
        if (_read != 0) {
            /*  lprintf("\n"); */
            //printf("TMP: %s\n", tmp);
            return tmp;
        } else {
            lprintf("Errore durante la read.\n");
        }
    } else {
        if(fd < 0){
            printf("ERRORE FD\n");
        }else{
        printf("Timed out\n");
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
    return id == HUB;
}

int hub_is_full(int pid, char *raw_info) {
    char **vars = split(raw_info);
    int count = atoi(vars[4]);
    free(vars);
    return count >= MAX_CHILDREN;
}

void hub_tree_print(char **vars) {
    if (strcmp(vars[0], HUB_S) == 0) {
        printf("Hub (PID: %s, Indice: %s), Stato: %s, Collegati: %s",
               vars[1], vars[2], atoi(vars[3]) ? "Acceso" : "Spento", vars[4]);
    } else {
        char device_name[MAX_BUF_SIZE];
        get_device_name(atoi(vars[0]), device_name);
        device_name[0] += 'A' - 'a';

        printf("%s, (PID %s, Indice %s)", device_name, vars[1], vars[2]);
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
            if (!strcmp(old, "<!") == 0 && to_be_printed > 0) {
                i = 0;
                hub_tree_spaces(level);
                hub_tree_print(vars);
                to_be_printed--;
            }
        } else if (strcmp(tokenizer, "!") == 0) {
            if (!strcmp(old, "!>") == 0 && to_be_printed > 0) {
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

int hub_tree_pid_finder(char *__buf, int id) {
    char *tokenizer = strtok(__buf, "|");
    char *old = NULL;
    char **vars;
    int i = 0;
    int to_be_printed = 1;
    /*printf("Level %d\n", level); */

    /* DISPOSITIVO; PID; ID */

    vars = malloc((FRIDGE_PARAMETERS + 4) * sizeof(*vars));

    while (tokenizer != NULL) {
        if (strcmp(tokenizer, "<!") == 0) {
            to_be_printed += atoi(old) - 1;
            i = 0;
            if (atoi(vars[2]) == id) {
                return atoi(vars[1]);
            }
        } else if (strcmp(tokenizer, "!>") == 0) {
            if (strcmp(old, "<!") == 0 && to_be_printed > 0) {
                i = 0;

                if (atoi(vars[2]) == id) {
                    return atoi(vars[1]);
                }
                to_be_printed--;
            }
        } else if (strcmp(tokenizer, "!") == 0) {
            if (to_be_printed > 0) {
                i = 0;

                if (atoi(vars[2]) == id) {
                    return atoi(vars[1]);
                }
                to_be_printed--;
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
    //printf("SHELLPID FUORI\n");
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