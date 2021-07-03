#include "shell.h"

int ppid;                        /* pid del padre */
int stato = 1;                   /* stato della centralina. */
int changed = 0;                 /* modifiche alla message queue? */
int children_pids[MAX_CHILDREN]; /* array contenenti i PID dei figli */
int fd;                          /* file descriptor */
int del_index = -1;
int max_index = 1;

int main(int argc, char *argv[]) {
    char(*buf)[MAX_BUF_SIZE] = malloc(MAX_BUF_SIZE * sizeof(char *)); /* array che conterrà i comandi da eseguire */
    char __out_buf[MAX_BUF_SIZE];                                     /* Per la stampa dell'output delle funzioni */
    char *name = get_shell_text();                                    /* Mostrato a ogni riga della shell */

    int cmd_n;        /* numero di comandi disponibili */
    int device_i = 0; /* indice progressivo dei dispositivi */

    char pipe[MAX_BUF_SIZE]; /* buffer messaggi della pipe */

    int j;
    key_t key;
    int msgid;
    key_t key_sh;
    int msgid_sh;

    char current_msg[MAX_BUF_SIZE] = "0|";

    char tmp_c[MAX_BUF_SIZE];

    char child[8];

    int i;

    signal(SIGUSR1, stop_sig);
    signal(SIGTERM, cleanup_sig);
    signal(SIGUSR2, link_child);
    signal(SIGINT, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, handle_sig);

    /* Inizializzo l'array dei figli */
    for (j = 0; j < MAX_CHILDREN; j++) {
        children_pids[j] = -1; /* se è -1 non contiene nulla */
    }

    if (argc != 2 || strcmp(argv[1], "--no-wrapper") != 0) {
        get_pipe_name((int)getpid(), pipe);
        mkfifo(pipe, 0666);
        fd = open(pipe, O_RDWR);

        /* PID del launcher. */
        ppid = atoi(argv[1]);

        /* Credo message queue tra shell e launcher */
        key = ftok("/tmp", 1000);
        msgid = msgget(key, 0666 | IPC_CREAT);
        message.mesg_type = 1;

        /*Creo message queue per comunicare shellpid */
        key_sh = ftok("/tmp", 2000);
        msgid_sh = msgget(key_sh, 0666 | IPC_CREAT);
        message.mesg_type = 1;

        sprintf(message.mesg_text, "%d", (int)getpid());
        msgsnd(msgid_sh, &message, MAX_BUF_SIZE, 0);
    }
    /* Ready */
    system("clear");

    while (1) {
            /* Scrive numero devices e elenco dei pid a launcher. */
            if ((argc != 2 || strcmp(argv[1], "--no-wrapper") != 0) && changed) {
                /* Ripulisco Forzatamente. */
                msgrcv(msgid, &message, MAX_BUF_SIZE, 1, IPC_NOWAIT);

                sprintf(tmp_c, "%d|", device_i);
                i = 0;
                while (i < device_i) {
                    sprintf(child, "%d|", children_pids[i]);
                    strcat(tmp_c, child);
                    i++;
                }
                sprintf(message.mesg_text, "%s", tmp_c);
                sprintf(current_msg, "%s", message.mesg_text);
                msgsnd(msgid, &message, MAX_BUF_SIZE, 0);
                changed = 0;
            
            
            /* stringa Centralina colorata */
            printf("\033[0;32m%s\033[0m:\033[0;31mCentralina\033[0m$ ", name);
            cmd_n = parse(buf, cmd_n); /* prende comando */

            if (strcmp(buf[0], "help") == 0) { /* guida ai comandi */
                printf(HELP_STRING);
            } else if (strcmp(buf[0], "list") == 0) { /* lista dispositivi */
                if (cmd_n != 0) {
                    printf(LIST_STRING); /* guida: list */
                } else {
                    __list(children_pids);
                }
            } else if (strcmp(buf[0], "info") == 0) { /* informazioni in base al pid */
                if (cmd_n != 1) {
                    printf(INFO_STRING); /* guida: info */
                } else {
                    __info(atoi(buf[1]), children_pids);
                }
            } else if (strcmp(buf[0], "switch") == 0) { /* modifica stato interruttori */
                if (cmd_n != 3) {
                    printf(SWITCH_STRING); /* giuda: switch */
                } else if (strcmp(buf[2], "riempimento") == 0) { /* possibile solo da launcher */
                    printf("Operazione permessa solo manualmente.\n");
                } else {
                    __switch_index(atoi(buf[1]), buf[2], buf[3], children_pids);
                }
            } else if (strcmp(buf[0], "add") == 0) { /* aggiunge device */
                if (cmd_n != 1) {
                    printf(ADD_STRING); /* giuda: add */
                } else {
                    changed = add_shell(buf, &device_i, children_pids, __out_buf);
                    printf("%s", __out_buf);
                    max_index ++;
                }
            } else if (strcmp(buf[0], "del") == 0) { /* elimina dispositivo */
                if (cmd_n != 1) {
                    printf(DEL_STRING); /* guida: del */
                } else {
                    del_direct(atoi(buf[1]), children_pids, __out_buf);
                    printf("%s", __out_buf);
                    del_index = atoi(buf[1]);
                }
            } else if (strcmp(buf[0], "link") == 0 && strcmp(buf[2], "to") == 0) { /* link tra dispositivi */
                if (cmd_n != 3) {
                    printf(LINK_STRING); /* giuda link */
                } else {
                    __link(atoi(buf[1]), atoi(buf[3]), children_pids);
                }
            } else if (strcmp(buf[0], "exit") == 0) { /* supponiamo che l'utente scriva solo "exit" per uscire */
                kill(ppid, SIGTERM); /* dice al padre cha sta per morire */
                break;
            } else if (strcmp(buf[0], "\0") == 0) { /* a capo a vuoto */
                continue;
            } else { /*tutto il resto */
                printf(UNKNOWN_COMMAND); /* comando non riconosciuto */
            }
        } else {
            /*Ripulisco forzatamente. */
            msgrcv(msgid, &message, MAX_BUF_SIZE, 1, IPC_NOWAIT);
            /*A centralina spenta continuo a scrivere il vecchio stato sulla messagequeue. */
            sprintf(message.mesg_text, "%s", current_msg);
            msgsnd(msgid, &message, MAX_BUF_SIZE, 0);
            getchar(); /*Per ignorare i comandi quando non accessa. */
        }
    }
    free(buf);
    return 0;
}

/* aggiungo dispositivo sotto la shell */
int add_shell(char buf[][MAX_BUF_SIZE], int *device_i, int *children_pids, char *__out_buf) {
    if (strcmp(buf[1], "bulb") == 0 || strcmp(buf[1], "fridge") == 0 || strcmp(buf[1], "window") == 0 || strcmp(buf[1], "hub") == 0 || strcmp(buf[1], "timer") == 0) {
        (*device_i)++;
        if (__add(buf[1], *device_i, children_pids, MAX_CHILDREN, __out_buf) == 0) {
            /* Non c'è spazio */
            (*device_i)--;
            return 0;
        } else {
            return 1;
        };
    } else {
        sprintf(__out_buf, "Dispositivi disponibili: <bulb/fridge/window/timer/hub>\n");
        return 0;
    }
}

/* quando arriva un segnale SIGTERM -> dice a tutti i figli che si sta spegnendo */
void cleanup_sig(int sig) {
    int i = 0;
    printf("Chiusura della centralina in corso...\n");
    for(i = 0; i < MAX_CHILDREN; i++){
        key_t key = ftok("/tmp/ipc/mqueues", i);
        int msgid = msgget(key, 0666 | IPC_CREAT);
        msgrcv(msgid, &message, sizeof(message), 1, IPC_NOWAIT);
    }
    kill(ppid, SIGTERM);
    kill(0, SIGKILL);
}

/* quando arriva un segnale SIGHUP -> dice al Launcher che è stata spenta */
void handle_sig(int sig) {
    kill(ppid, SIGTERM);
}

/* quando arriva un segnale SIGUSER1 -> cambia di stato la centralina */
void stop_sig(int sig) {
    if (stato) {
        stato = 0;
        printf("La centralina è stata spenta. Nessun comando sarà accettato.\n");
    } else {
        stato = 1;
        system("clear");
        printf("\nLa centralina è accesa. Premi Invio per proseguire.\n");
    }
}

/* quando arriva SIGUSR2 -> link di un dispositivo come figlio */
void link_child(int signal) {
    char **vars;
    int q, j;
    int n_devices, __count;
    char** son_j;
    char n_dev_str[MAX_BUF_SIZE];
    char tmp_buf[MAX_BUF_SIZE];
    if (del_index != -1) {
    key_t key_del = ftok("/tmp/ipc/mqueues", del_index);
    int msgid_del = msgget(key_del, 0666 | IPC_CREAT);
    

   int ret = msgrcv(msgid_del, &message, sizeof(message), 1, IPC_NOWAIT);
   if (ret != -1) {
        q = 0;
        while (!(message.mesg_text[q] == '-')) {
            n_dev_str[q] = message.mesg_text[q];
            q++;
        }
        n_dev_str[q] = '\0';
        n_devices = atoi(n_dev_str);
        if (n_devices > 0) {
            __count = n_devices;
            sprintf(tmp_buf, "%s", message.mesg_text);
            vars = NULL;
            vars = split_sons(tmp_buf, __count);
            j = 0;
            while (j <= __count) {
                if (j >= 1) {
                    printf("\nVars %d: %s\n", j, vars[j]);
                    son_j = split(vars[j]);
                    __add_ex(son_j, children_pids, MAX_CHILDREN);
                    printf("\nSon added to shell\n");
                }
                j++;
            }
        }
   }
    del_index  = -1;
    }
}
