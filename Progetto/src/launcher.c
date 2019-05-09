#include "launcher.h"

pid_t shell_pid = -1;
int n_devices = 0;
int emergencyid;

int main(int argc, char *argv[]) {
    signal(SIGTERM, handle_sig);
    signal(SIGHUP, handle_sighup);
    signal(SIGINT, handle_sigint);

    char(*buf)[MAX_BUF_SIZE] = malloc(MAX_BUF_SIZE * sizeof(char *));  // array che conterrà i comandi da eseguire
    char ch;                                                           // carattere usato per la lettura dei comandi
    int ch_i;                                                          // indice del carattere corrente

    int cmd_n;  // numero di comandi disponibili

    int device_pids[MAX_CHILDREN];  // array contenenti i PID dei figli
    int j;

    for (j = 0; j < MAX_CHILDREN; j++) {
        device_pids[j] = -1;  // se è -1 non contiene nulla
    }

    system("clear");
    //Creo PIPE verso shell.
    char *shpm = "/tmp/myshpm";
    mkfifo(shpm, 0666);
    char *name = get_shell_text();
    //CREO MESSAGE QUEUE TRA SHELL E LAUNCHERRRRRRRRRRRRRRR
    key_t key;
    key = ftok("progfile", 65);
    int msgid;
    msgid = msgget(key, 0666 | IPC_CREAT);
    emergencyid = msgid;
    while (1) {
        //Leggo il numero di devices presenti.
        if (shell_pid > 0) {
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
                int __count = n_devices;
                char tmp_buf[MAX_BUF_SIZE];
                sprintf(tmp_buf, "%s", message.mesg_text);
                char *tokenizer = strtok(tmp_buf, "|");
                char **vars = malloc(__count * sizeof(char *));
                int j = 0;
                while (tokenizer != NULL && j <= __count) {
                    vars[j++] = tokenizer;
                    tokenizer = strtok(NULL, "|");
                    if (j >= 2) {
                        device_pids[j - 1] = atoi(vars[j - 1]);
                    }
                }
            }
        } else {
            n_devices = 0;
        }

        //Stampa del nome utente
        printf("\e[92m%s\e[39m:\e[34mLauncher\033[0m$ ", name);
        //Sono presi i valori in input e divisi in buffer[0...n] per ogni parola.
        // Parser dei comandi
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

        if (strcmp(buf[0], "exit") == 0) {  // supponiamo che l'utente scriva solo "exit" per uscire
            //Se la shell è aperta => shell_pid != -1 mando un SIGTERM per chiuderla.
            if (shell_pid != -1) {
                kill(shell_pid, SIGTERM);
            }
            break;
        } else if (strcmp(buf[0], "\0") == 0) {  //a capo a vuoto
            continue;
        } else if (strcmp(buf[0], "help") == 0) {  // guida
            printf(HELP_STRING_LAUNCHER);
        } else if (strcmp(buf[0], "info") == 0) {  // info su dispositivo
            if (cmd_n != 1) {
                printf("Sintassi: info <device>\n");
            }
            ////CODICE DUPLICATOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO
            else {
                info_launcher(buf, msgid, device_pids);
            }
        } else if (strcmp(buf[0], "user") == 0) {  //I comandi da launcher fuorchè per help ed exit devono cominciare per user.
            if (strcmp(buf[1], "turn") == 0 && strcmp(buf[2], "shell") == 0) {
                if (cmd_n != 3) {  //Controllo correttezza nel conteggio degli argomenti.
                    printf("Sintassi: user turn shell <pos>\n");
                } else {
                    if (strcmp(buf[3], "on") == 0 && shell_pid == -1) {  //Se non è ancora accesa => shell_pid == -1
                        pid_t pid = fork();
                        if (pid < 0) {
                            printf("Errore durante il fork\n");
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
                                int fd = open(shpm, O_WRONLY);
                                char tmp[16] = "Errore";
                                write(fd, tmp, 16);
                                close(fd);
                            }
                        } else if (pid > 0) {
                            //Legge il contenuto della pipe => Se = "Errore" la finestra è stata aperta.
                            int fd = open(shpm, O_RDONLY);
                            char tmp[16];
                            read(fd, tmp, 16);
                            if (strcmp(tmp, "Errore") == 0) {
                                printf("Errore nell'apertura della Shell\n");
                            } else {
                                shell_pid = atoi(tmp);
                            }
                            close(fd);
                            system("clear");
                            printf("La centralina è aperta\n");
                            continue;
                        }
                    } else if (strcmp(buf[3], "off") == 0 && shell_pid != -1) {
                        kill(shell_pid, SIGTERM);
                        shell_pid = -1;
                        continue;
                    } else if (strcmp(buf[3], "on") == 0 && shell_pid != -1) {
                        printf("Centralina già accesa\n");
                    } else if (strcmp(buf[3], "off") == 0 && shell_pid == -1) {
                        printf("Centralina già spenta\n");
                    } else {
                        printf("Comando non riconosciuto\n");
                    }
                }
            }
        } else if (strcmp(buf[0], "switch") == 0) {
            if (cmd_n != 3) {
                printf("Sintassi: switch <id> <label> <pos>\nInterruttori disponibili:\n    bulb: accensione\n");
            } else {
                switch_launcher(buf, msgid, device_pids);
            }
        } else if (strcmp(buf[0], "restart") == 0) {
            char *const args[] = {NULL};
            int pid = fork();
            if (pid == 0) {
                execvp("make build", args);
                exit(0);
            } else {
                wait(NULL);
                execvp("./run", args);
            }
            exit(0);
        } else {  //tutto il resto
            printf("Comando non riconosciuto. Usa help per visualizzare i comandi disponibili\n");
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
    printf("La centralina è stata chiusa, Premere Invio per proseguire\n");
    if (shell_pid != -1) {
        kill(shell_pid, SIGTERM);
    }
    exit(0);
}

void handle_sig(int signal) {
    system("clear");
    printf("La centralina è stata chiusa, Premere Invio per proseguire\n");
    shell_pid = -1;
}

void handle_sigint(int signal) {
    msgctl(emergencyid, IPC_RMID, NULL);
    if (shell_pid = ! - 1) {
        kill(shell_pid, SIGTERM);
    }
    exit(0);
}

void switch_launcher(char buf[][MAX_BUF_SIZE], int msgid, int *device_pids) {
    //CODICE DUPLICATO
    if (shell_pid > 0) {
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
            int __count = n_devices;
            char tmp_buf[MAX_BUF_SIZE];
            sprintf(tmp_buf, "%s", message.mesg_text);
            char *tokenizer = strtok(tmp_buf, "|");
            char **vars = malloc(__count * sizeof(char *));
            int j = 0;
            while (tokenizer != NULL && j <= __count) {
                vars[j++] = tokenizer;
                tokenizer = strtok(NULL, "|");
                if (j >= 2) {
                    device_pids[j - 1] = atoi(vars[j - 1]);
                }
            }
        }
    }
    if (shell_pid > 0) {
        if (atoi(buf[1]) <= n_devices) {
            // Chiamata a util.c
            __switch(buf, device_pids);
        } else {
            printf("ID non presente\n");
        }

    } else {
        printf("Azione non disponibile a centralina spenta!\n");
    }
}

void info_launcher(char buf[][MAX_BUF_SIZE], int msgid, int *device_pids) {
    if (shell_pid > 0) {
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
            int __count = n_devices;
            char tmp_buf[MAX_BUF_SIZE];
            sprintf(tmp_buf, "%s", message.mesg_text);
            char *tokenizer = strtok(tmp_buf, "|");
            char **vars = malloc(__count * sizeof(char *));
            int j = 0;
            while (tokenizer != NULL && j <= __count) {
                vars[j++] = tokenizer;
                tokenizer = strtok(NULL, "|");
                if (j >= 2) {
                    device_pids[j - 1] = atoi(vars[j - 1]);
                }
            }
        }

        __info(buf, device_pids);
    } else {
        printf("La centralina è spenta\n");
    }
}