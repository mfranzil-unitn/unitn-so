#include "shell.h"

int ppid;
int changed = 0;
int stato = 1;  //Stato della centralina.

int main(int argc, char *argv[]) {
    signal(SIGINT, stop_sig);
    signal(SIGTERM, cleanup_sig);
    signal(SIGUSR1, SIG_IGN);

    char(*buf)[MAX_BUF_SIZE] = malloc(MAX_BUF_SIZE * sizeof(char *));  // array che conterrà i comandi da eseguire

    char ch;   // carattere usato per la lettura dei comandi
    int ch_i;  // indice del carattere corrente

    int cmd_n;  // numero di comandi disponibili

    int j;

    int device_i = 0;                 // indice progressivo dei dispositivi
    int children_pids[MAX_CHILDREN];  // array contenenti i PID dei figli

    for (j = 0; j < MAX_CHILDREN; j++) {
        children_pids[j] = -1;  // se è -1 non contiene nulla
    }

    system("clear");
    char *name = get_shell_text();

    //PID del launcher.
    ppid = atoi(argv[1]);

    //Pipe per comunicazione del numero di devices.
    int fd = open(SHPM, O_WRONLY);

    //Scrivo il pid della centralina, dato che non è figlia diretta di program manager, sulla pipe.
    char str[16];
    sprintf(str, "%d", (int)getpid());
    write(fd, str, 16);
    close(fd);
    ////FINE MODIFICAAAAAAAAAAAAAAAAAAAAAAA

    /////MODIFICAAAAAAAAAAAAAAAAAAAAA
    signal(SIGHUP, handle_sig);

    //CREO MESSAGE QUEUE TRA SHELL E LAUNCHERRRRRRRRRRRRRRR
    key_t key;
    key = ftok("progfile", 65);
    int msgid;
    msgid = msgget(key, 0666 | IPC_CREAT);
    message.mesg_type = 1;

    char current_msg[MAX_BUF_SIZE] = "0|";

    //setpgid(0, getpid());

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
                kill(ppid, SIGTERM);
                break;
            } else if (strcmp(buf[0], "\0") == 0) {  // a capo a vuoto
                continue;
            } else if (strcmp(buf[0], "help") == 0) {  // guida
                printf("%s", HELP_STRING);
            } else if (strcmp(buf[0], "list") == 0) {
                list(buf, children_pids);
            } else if (strcmp(buf[0], "info") == 0) {
                if (cmd_n != 1) {
                    printf("Sintassi: info <device>\n");
                } else {
                    __info(buf, children_pids);
                }
            } else if (strcmp(buf[0], "switch") == 0) {
                if (cmd_n != 3) {
                    printf(SWITCH_STRING);
                } else {
                    __switch(buf, children_pids);
                }
            } else if (strcmp(buf[0], "add") == 0) {
                if (cmd_n != 1) {
                    printf(ADD_STRING);
                } else {
                    add(buf, &device_i, children_pids);
                    continue;
                }
            } else if (strcmp(buf[0], "del") == 0) {
                if (cmd_n != 1) {
                    printf(DEL_STRING);
                } else {
                    del(buf, children_pids);
                    continue;
                }
            } else {  //tutto il resto
                printf("Comando non riconosciuto. Usa help per visualizzare i comandi disponibili\n");
            }
        } else {
            getchar();  //Per ignorare i comandi quando non accessa.
        }
    }
    free(buf);
    return 0;
}

void list(char buf[][MAX_BUF_SIZE], int *children_pids) {
    // prende come input l'indice/nome del dispositivo, ritorna il PID
    char *pipe_str = NULL;

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
            printf("Dispositivo: %s, PID %s, nome %s\n", vars[0], vars[1], vars[2]);
            // Pulizia
            free(vars);
            free(pipe_str);
            close(fd);
        }
    }

    /*kill(0, SIGUSR1);
    char tmp[MAX_BUF_SIZE];
    char *pipe_str = NULL;

    int i;    
    for (i = 0; i < MAX_CHILDREN; i++) {
        if (children_pids[i] != -1) {
            pipe_str = get_pipe_name(children_pids[i]);
            int fd = open(pipe_str, O_RDONLY);
            if (fd > 0) {
                read(fd, tmp, MAX_BUF_SIZE);
                char **vars = split(tmp);
                printf("Dispositivo: %s, PID %s, nome %s\n", vars[0], vars[1], vars[2]);
                // Pulizia
                free(vars);
                free(pipe_str);
                close(fd);
                tmp[0] = '\0';
            }
        }
    }*/
}

void add(char buf[][MAX_BUF_SIZE], int *device_i, int *children_pids) {
    if (strcmp(buf[1], "bulb") == 0 || strcmp(buf[1], "fridge") == 0 || strcmp(buf[1], "window") == 0) {
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
                printf("Non c'è più spazio! Rimuovi qualche dispositivo.\n");
                return;
            }
        } else {
            actual_index = *device_i - 1;
        }

        pid_t pid = fork();
        if (pid == 0) {  // Figlio
            // Apro una pipe per padre-figlio
            char *pipe_str = get_pipe_name(getpid());
            mkfifo(pipe_str, 0666);

            // Conversione a stringa dell'indice
            char *index_str = malloc(4 * sizeof(char));
            sprintf(index_str, "%d", *device_i);

            char program_name[MAX_BUF_SIZE / 4];
            sprintf(program_name, "./%s%s", DEVICES_POSITIONS, buf[1]);

            // Metto gli argomenti in un array e faccio exec
            char *const args[] = {program_name, index_str, pipe_str, NULL};
            execvp(args[0], args);

            exit(0);
        } else {  // Padre
            children_pids[actual_index] = pid;

            char device_name[MAX_BUF_SIZE];
            get_device_name(atoi(buf[1]), device_name);

            printf("Aggiunto un dispositivo di tipo %s con PID %i e indice %i\n", device_name, pid, *device_i);
            changed = 1;
            return;
        }
    } else {
        printf("Dispositivo non ancora supportato\n");
    }
}

void del(char buf[][MAX_BUF_SIZE], int *children_pids) {
    int pid = get_device_pid(atoi(buf[1]), children_pids);

    if (pid == -1) {
        printf("Errore! Non esiste questo dispositivo.\n");
        return;
    }

    char *pipe_str = get_pipe_name(pid);
    char tmp[MAX_BUF_SIZE];  // dove ci piazzo l'output della pipe
    char **vars = NULL;

    if (kill(pid, SIGUSR1) != 0) {
        printf("Errore! Sistema: codice errore %i\n", errno);
        return;
    }

    int fd = open(pipe_str, O_RDONLY);
    read(fd, tmp, MAX_BUF_SIZE);

    vars = split(tmp);

    char device_name[MAX_BUF_SIZE];
    get_device_name(atoi(vars[0]), device_name);
    device_name[0] += 'A' - 'a';

    printf("Rimozione in corso...\nDispositivo di tipo %s con PID %s e indice %s rimosso.\n",
           device_name, vars[1], vars[2]);

    close(fd);
    free(pipe_str);
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

void cleanup_sig(int sig) {
    printf("Chiusura della centralina in corso...\n");
    kill(ppid, SIGTERM);
    kill(0, 9);
}

void handle_sig(int sig) {
    kill(ppid, SIGTERM);
}

void stop_sig(int sig) {
    printf("STOP_SIG\n");
    if (stato) {
        stato = 0;
        printf("La centralina è stata spenta. Nessun comando sarà accettato\n");
    } else {
        stato = 1;
        system("clear");
        printf("\nLa centralina è accessa. Premi Invio per proseguire.\n");
    }
}