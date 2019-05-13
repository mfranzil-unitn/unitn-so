#include "launcher.h"

extern int print_mode;

pid_t shell_pid = -1;
int n_devices = 0;
int emergencyid;
int shell_on = 0;

int main(int argc, char *argv[]) {
    signal(SIGTERM, handle_sig);
    signal(SIGHUP, handle_sighup);
    signal(SIGINT, handle_sigint);

    char(*buf)[MAX_BUF_SIZE] = malloc(MAX_BUF_SIZE * sizeof(char *));  // array che conterrà i comandi da eseguire

    int cmd_n;  // numero di comandi disponibili

    int device_pids[MAX_CHILDREN];  // array contenenti i PID dei figli

    print_mode = 1;  // abilito la stampa

    int j;
    for (j = 0; j < MAX_CHILDREN; j++) {
        // Inizializzo l'array dei figli
        device_pids[j] = -1;  // se è -1 non contiene nulla
    }

    if (argc != 2 || strcmp(argv[1], "--no-clear") != 0) {
        // Parametro opzionale
        system("clear");
    }

    char *name = get_shell_text();

    // Creo message queue tra shell e launcher.
    key_t key;
    key = ftok("progfile", 65);
    int msgid;
    msgid = msgget(key, 0666 | IPC_CREAT);
    // Ripulisco inizialmente per evitare errori.
    msgrcv(msgid, &message, sizeof(message), 1, IPC_NOWAIT);
    emergencyid = msgid;

    while (1) {
        //Leggo il numero di devices presenti.
        if (shell_pid > 0 && shell_on) {
            read_msgqueue(msgid, device_pids);
        } else {
            n_devices = 0;
        }

        // Stampa del nome utente
        cprintf("\e[92m%s\e[39m:\e[34mLauncher\033[0m$ ", name);
        cmd_n = parse(buf, cmd_n);

        if (strcmp(buf[0], "exit") == 0) {
            // supponiamo che l'utente scriva solo "exit" per uscire
            //Se la shell è aperta => shell_pid != -1 mando un SIGTERM per chiuderla.
            char c = '\0';
            cprintf("Sei sicuro di voler uscire dal launcher? Tutte le modifiche verranno perse!");

            while (1) {
                printf("[s/n]: ");
                if (scanf(" %c", &c) != 1) {
                    printf("Errore durante la lettura.\n");
                    continue;
                }

                if (c == 's' || c == 'S') {
                    //Eliminazione messagequeue verso shell.
                    msgctl(msgid, IPC_RMID, NULL);
                    free(buf);
                    if (shell_pid != -1) {
                        kill(shell_pid, SIGTERM);
                    }
                    return 0;
                } else if (c == 'n' || c == 'N') {
                    break;
                } else {
                    printf("Inserisci [s]ì o [n]o.\n");
                }
            }
        } else if (strcmp(buf[0], "\0") == 0) {  //a capo a vuoto
            continue;
        } else if (strcmp(buf[0], "help") == 0) {  // guida
            cprintf(HELP_STRING_LAUNCHER);
        } else if (strcmp(buf[0], "info") == 0) {  // info su dispositivo
            if (cmd_n != 1) {
                cprintf(INFO_STRING);
            } else {
                info_launcher(buf, msgid, device_pids);
            }
        } else if (strcmp(buf[0], "switch") == 0) {
            if (cmd_n != 3) {
                cprintf(SWITCH_STRING);
            } else {
                switch_launcher(buf, msgid, device_pids);
            }
        } else if (strcmp(buf[0], "user") == 0) {  //I comandi da launcher fuorchè per help ed exit devono cominciare per user.
            if (cmd_n != 3) {                      //Controllo correttezza nel conteggio degli argomenti.
                cprintf(USER_STRING);
            } else {
                user_launcher(buf, msgid, device_pids);
            }
        } else if (strcmp(buf[0], "restart") == 0) {
            cprintf("Riavvio sta dando problemi, non usarmi\n");
            continue;
            //  int pid = fork();
            //if (pid == 0) {
            if (shell_pid != -1) {
                kill(shell_pid, SIGTERM);
            }

            msgctl(msgid, IPC_RMID, NULL);
            free(buf);
            cprintf("Riavvio in corso...\n");
            system("make build");
            system("./run --no-clear");
            //  } else {
            //    wait(NULL);
            //  }
            //  exit(0);
        } else {  //tutto il resto
            cprintf("Comando non riconosciuto. Usa help per visualizzare i comandi disponibili\n");
        }
    }

    // to destroy the message queue
    msgctl(msgid, IPC_RMID, NULL);
    free(buf);
    return 0;
}

//Da togliere, ma aspetta.
void handle_sighup(int signal) {
    system("clear");
    cprintf("La centralina è stata chiusa, Premere Invio per proseguire\n");
    if (shell_pid != -1) {
        kill(shell_pid, SIGTERM);
    }
    exit(0);
}

void handle_sig(int signal) {
    system("clear");
    cprintf("La centralina è stata chiusa, Premere Invio per proseguire\n");
    shell_pid = -1;
}

void handle_sigint(int signal) {
    msgctl(emergencyid, IPC_RMID, NULL);
    if (shell_pid != -1) {
        kill(shell_pid, SIGTERM);
    }
    exit(0);
}

void switch_launcher(char buf[][MAX_BUF_SIZE], int msgid, int *device_pids) {
    if (shell_pid > 0) {
        //Switch possibile anche a centralina spenta? Altrimenti aggiungo && shell_on
        read_msgqueue(msgid, device_pids);
        if (atoi(buf[1]) <= n_devices) {
            // Chiamata a util.c
            __switch(atoi(buf[1]), buf[2], buf[3], device_pids);
        } else {
            cprintf("ID non presente\n");
        }

    } else {
        cprintf("Azione non disponibile a centralina spenta!\n");
    }
}

void read_msgqueue(int msgid, int *device_pids) {
    int ret = msgrcv(msgid, &message, sizeof(message), 1, IPC_NOWAIT);
    if (ret != -1) {
        int q = 0;
        char n_dev_str[100];
        while (!(message.mesg_text[q] == '|')) {
            n_dev_str[q] = message.mesg_text[q];
            q++;
        }
        n_dev_str[q] = '\0';
        n_devices = atoi(n_dev_str);
        if (n_devices > 0) {
            int __count = n_devices;
            char tmp_buf[MAX_BUF_SIZE];
            sprintf(tmp_buf, "%s", message.mesg_text);
            char **vars = NULL;
            vars = split_fixed(tmp_buf, __count);
            int j = 0;
            while (j <= __count) {
                if (j >= 1) {
                    device_pids[j - 1] = atoi(vars[j]);
                }
                j++;
            }
        }
    }
}

void info_launcher(char buf[][MAX_BUF_SIZE], int msgid, int *device_pids) {
    if (shell_pid > 0 && shell_on) {
        read_msgqueue(msgid, device_pids);
        __info(atoi(buf[1]), device_pids);
    } else {
        cprintf("La centralina è spenta\n");
    }
}

void user_launcher(char buf[][MAX_BUF_SIZE], int msgid, int *device_pids) {
    if (strcmp(buf[1], "turn") != 0 || strcmp(buf[2], "shell") != 0) {
        cprintf("Sintassi: user turn shell <pos>\n");
    }

    if (strcmp(buf[3], "on") == 0 && shell_pid == -1) {  //Se non è ancora accesa => shell_pid == -1
        pid_t pid = fork();
        if (pid < 0) {
            cprintf("Errore durante il fork\n");
            exit(1);
        }
        if (pid == 0) {  //Processo figlio che aprirà terminale e lancerà la shell.

            //Sarà passato per argomento alla shell.
            int ppid = (int)getppid();

            //Eseguibili sono in bin apro terminale parallelo.
            char tmp[50] = "./bin/shell ";
            char stringpid[6];
            sprintf(stringpid, "%d", ppid);
            strcat(tmp, stringpid);
            if (execl("/usr/bin/gnome-terminal", "gnome-terminal", "-e", tmp, NULL) == -1) {
                sprintf(message.mesg_text, "%s", "Errore");
                msgsnd(msgid, &message, MAX_BUF_SIZE, 0);
            }
        } else if (pid > 0) {
            //Legge il contenuto della pipe => Se = "Errore" la finestra è stata aperta.
            msgrcv(msgid, &message, sizeof(message), 1, 0);
            if (strcmp(message.mesg_text, "Errore") == 0) {
                cprintf("Errore nell'apertura della shell\n");
            } else {
                shell_pid = atoi(message.mesg_text);
            }
            shell_on = 1;
            system("clear");
            cprintf("La centralina è aperta\n");
            return;
        }
    } else if (strcmp(buf[3], "off") == 0 && shell_pid != -1) {
        if(shell_on) {
            kill(shell_pid, SIGINT);
            shell_on = 0;
        }
        else{
            printf("Centralina già spenta.\n");
        }
        return;
    } else if (strcmp(buf[3], "on") == 0 && shell_pid != -1) {
        if(shell_on == 0) {
            kill(shell_pid, SIGINT);
            shell_on = 1;
        }else{
            printf("Centralina già accesa\n");
        }
    } else if (strcmp(buf[3], "off") == 0 && shell_pid == -1) {
        cprintf("Centralina già spenta\n");
    } else {
        cprintf("Comando non riconosciuto\n");
    }
}