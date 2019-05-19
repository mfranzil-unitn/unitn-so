#include "shell.h"

// extern int print_mode;

int ppid;
int stato = 1;                    //Stato della centralina.
int changed = 0;                  // Modifiche alla message queue?
int children_pids[MAX_CHILDREN];  // array contenenti i PID dei figli
int fd;

int main(int argc, char *argv[]) {
    signal(SIGUSR1, stop_sig);
    signal(SIGTERM, cleanup_sig);
    signal(SIGUSR2, link_child);
    signal(SIGINT, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    char(*buf)[MAX_BUF_SIZE] = malloc(MAX_BUF_SIZE * sizeof(char *));  // array che conterrà i comandi da eseguire
    char __out_buf[MAX_BUF_SIZE];                                      // Per la stampa dell'output delle funzioni
    char *name = get_shell_text();                                     // Mostrato a ogni riga della shell

    int cmd_n;         // numero di comandi disponibili
    int device_i = 0;  // indice progressivo dei dispositivi

    char pipe[MAX_BUF_SIZE];
    get_pipe_name((int)getpid(), pipe);
    mkfifo(pipe, 0666);
    fd = open(pipe, O_RDWR);

    // Inizializzo l'array dei figli
    int j;
    for (j = 0; j < MAX_CHILDREN; j++) {
        children_pids[j] = -1;  // se è -1 non contiene nulla
    }

    // PID del launcher.
    ppid = atoi(argv[1]);

    signal(SIGHUP, handle_sig);

    // Credo message queue tra shell e launcher
    key_t key;
    key = ftok("/tmp", 10);
    int msgid;
    msgid = msgget(key, 0666 | IPC_CREAT);
    message.mesg_type = 1;

    //Creo message queue per comunicare shellpid
    key_t key_sh;
    key_sh = ftok("/tmp", 20);
    int msgid_sh;
    msgid_sh = msgget(key_sh, 0666 | IPC_CREAT);
    message.mesg_type = 1;

    char current_msg[MAX_BUF_SIZE] = "0|";
    sprintf(message.mesg_text, "%d", (int)getpid());
    msgsnd(msgid, &message, MAX_BUF_SIZE, 0);

    // Ready
    system("clear");

    while (1) {
        if (stato) {
            //Scrive numero devices e elenco dei pid a launcher.
            if (changed) {
                //Ripulisco Forzatamente.
                msgrcv(msgid, &message, MAX_BUF_SIZE, 1, IPC_NOWAIT);

                char tmp_c[MAX_BUF_SIZE];
                sprintf(tmp_c, "%d|", device_i);
                char child[8];
                int i = 0;
                while (i < device_i) {
                    sprintf(child, "%d|", children_pids[i]);
                    strcat(tmp_c, child);
                    i++;
                }
                sprintf(message.mesg_text, "%s", tmp_c);
                sprintf(current_msg, "%s", message.mesg_text);
                msgsnd(msgid, &message, MAX_BUF_SIZE, 0);
                changed = 0;
            } else {
                //Ripulisco forzatamente.
                msgrcv(msgid, &message, MAX_BUF_SIZE, 1, IPC_NOWAIT);
                sprintf(message.mesg_text, "%s", current_msg);
                msgsnd(msgid, &message, MAX_BUF_SIZE, 0);
            }

            printf("\e[92m%s\e[39m:\e[31mCentralina\033[0m$ ", name);
            cmd_n = parse(buf, cmd_n);

            if (strcmp(buf[0], "help") == 0) {  // guida
                printf(HELP_STRING);
            } else if (strcmp(buf[0], "list") == 0) {
                if (cmd_n != 0) {
                    printf(LIST_STRING);
                } else {
                    __list(children_pids);
                }
            } else if (strcmp(buf[0], "info") == 0) {
                if (cmd_n != 1) {
                    printf(INFO_STRING);
                } else {
                    __info(atoi(buf[1]), children_pids);
                }
            } else if (strcmp(buf[0], "switch") == 0) {
                if (cmd_n != 3) {
                    printf(SWITCH_STRING);
                } else if (strcmp(buf[2], "riempimento") == 0) {
                    printf("Operazione permessa solo manualmente.\n");
                } else {
                    __switch(atoi(buf[1]), buf[2], buf[3], children_pids);
                }
            } else if (strcmp(buf[0], "add") == 0) {
                if (cmd_n != 1) {
                    printf(ADD_STRING);
                } else {
                    changed = add_shell(buf, &device_i, children_pids, __out_buf);
                    printf("%s", __out_buf);
                }
            } else if (strcmp(buf[0], "del") == 0) {
                if (cmd_n != 1) {
                    printf(DEL_STRING);
                } else {
                    __del(atoi(buf[1]), children_pids, __out_buf);
                    printf("%s", __out_buf);
                }
            } else if (strcmp(buf[0], "link") == 0) {
                if (cmd_n != 3) {
                    printf(LINK_STRING);
                } else {
                    __link(atoi(buf[1]), atoi(buf[3]), children_pids);
                }
            } else if (strcmp(buf[0], "exit") == 0) {  // supponiamo che l'utente scriva solo "exit" per uscire
                kill(ppid, SIGTERM);
                break;
            } else if (strcmp(buf[0], "\0") == 0) {  // a capo a vuoto
                continue;
            } else {  //tutto il resto
                printf("Comando non riconosciuto. Usa help per visualizzare i comandi disponibili\n");
            }
        } else {
            //Ripulisco forzatamente.
            msgrcv(msgid, &message, MAX_BUF_SIZE, 1, IPC_NOWAIT);
            //A centralina spenta continuo a scrivere il vecchio stato sulla messagequeue.
            sprintf(message.mesg_text, "%s", current_msg);
            msgsnd(msgid, &message, MAX_BUF_SIZE, 0);
            getchar();  //Per ignorare i comandi quando non accessa.
        }
    }
    free(buf);
    return 0;
}

int add_shell(char buf[][MAX_BUF_SIZE], int *device_i, int *children_pids, char *__out_buf) {
    if (strcmp(buf[1], "bulb") == 0 || strcmp(buf[1], "fridge") == 0 || strcmp(buf[1], "window") == 0 || strcmp(buf[1], "hub") == 0) {
        (*device_i)++;
        if (__add(buf[1], *device_i, children_pids, __out_buf) == 0) {
            (*device_i)--;
            return 0;
        } else {
            return 1;
        };
    } else {
        sprintf(__out_buf, "Dispositivo non ancora supportato\n");
        return 0;
    }
}

void cleanup_sig(int sig) {
    printf("Chiusura della centralina in corso...\n");
    kill(ppid, SIGTERM);
    kill(0, 9);
}

void handle_sig(int sig) {
    kill(ppid, SIGTERM);
}

void stop_sig(int sig) {
    if (stato) {
        stato = 0;
        printf("La centralina è stata spenta. Nessun comando sarà accettato\n");
    } else {
        stato = 1;
        system("clear");
        printf("\nLa centralina è accessa. Premi Invio per proseguire.\n");
    }
}

void link_child(int signal) {
    printf("Link_Child\n");
    //Analogamente ad Hub
    char *tmp = malloc(MAX_BUF_SIZE * sizeof(tmp));
    read(fd, tmp, MAX_BUF_SIZE);
    printf("End Read: %s\n\n", tmp);

    int code = tmp[0] - '0';
    if (code == 1) {
        tmp = tmp + 2;
        char **vars = split(tmp);
        __add_ex(vars, children_pids);
        free(vars);
        free(tmp - 2);
    }
    //close(fd);
}
