#include "shell.h"
#include "util.h"

// CENTRALINA = 0
// BULB = 1
// FRIDGE = 2
// WINDOW = 3

int main(int argc, char *argv[]) {
    signal(SIGINT, cleanup_sig);
    signal(SIGTERM, cleanup_sig);
    signal(SIGUSR1, SIG_IGN);

    char(*buf)[MAX_BUF_SIZE] = malloc(MAX_BUF_SIZE * sizeof(char *));  // array che conterrà i comandi da eseguire

    char ch;   // carattere usato per la lettura dei comandi
    int ch_i;  // indice del carattere corrente

    int cmd_n;  // numero di comandi disponibili

    int j;

    int device_i = 0;        // indice progressivo dei dispositivi
    int children_pids[100];  // array contenenti i PID dei figli

    for (j = 0; j < MAX_CHILDREN; j++) {
        children_pids[j] = -1;  // se è -1 non contiene nulla
    }

    system("clear");
    char *name = getUserName();

    /*
    //PID del launcher.
    int ppid = atoi(argv[1]);

    //Creo FIFO da shell a launcher.
    char *shpm = "/tmp/myshpm";
    mkfifo(shpm, 0666);
    int fd = open(shpm, O_WRONLY);

    //Scrivo il pid della centralina, dato che non è figlia diretta di program manager, sulla pipe.
    char str[16];
    sprintf(str, "%d", (int)getpid());
    write(fd, str, 16);
    close(fd);
    ////FINE MODIFICAAAAAAAAAAAAAAAAAAAAAAA

    /////MODIFICAAAAAAAAAAAAAAAAAAAAA
    signal(SIGINT, handle_sig);
    signal(SIGHUP, handle_sig);

*/

    //setpgid(0, getpid());

    while (1) {
        printf("\e[92m%s\e[39m:\e[31mCentralina\033[0m$ ", name);

        // Parser dei comandi
        ch = ' ';
        ch_i = -1;
        cmd_n = 0;
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

        //   for (int k = cmd_n; k >= 0; k--) {
        //       printf(buf[k]);
        //   }

        if (strcmp(buf[0], "exit") == 0) {  // supponiamo che l'utente scriva solo "exit" per uscire
            break;
        } else if (strcmp(buf[0], "\0") == 0) {  // a capo a vuoto
            //return; Hai scritto Return, credo sia un continue, sbaglio?
            continue;
        } else if (strcmp(buf[0], "help") == 0) {  // guida
            printf("%s", HELP_STRING);
        } else if (strcmp(buf[0], "list") == 0) {
            list(buf, children_pids);
        } else if (strcmp(buf[0], "info") == 0) {
            if (cmd_n != 1) {
                printf("Sintassi: info <device>\n");
            } else {
                info(buf, children_pids);
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
            }
        } else if (strcmp(buf[0], "restart") == 0) {
            char *const args[] = {NULL};
            int pid = fork();
            if (pid == 0) {
                execvp("make", args);
                exit(0);
            } else {
                wait(NULL);
                signal(SIGQUIT, SIG_IGN);
                kill(-getppid(), SIGQUIT);
                execvp("./bin/shell", args);
            }
            exit(0);
        } else {  //tutto il resto
            printf("Comando non riconosciuto. Usa help per visualizzare i comandi disponibili\n");
        }
    }
    free(buf);
    return 0;
}

char *pipename(int pid) {
    char *pipe_str = malloc(4 * sizeof(char));
    sprintf(pipe_str, "/tmp/ipc/%i", pid);
    return pipe_str;
}

int get_by_index(int in, int *children_pids) {
    // manda un messaggio in broadcast per inviare le informazioni di tutti sulle rispettive pipe.
    // per adesso non ha molto senso, in quando dato l'indice i children_pids[i] ha il pid, ma questa info
    // non possiamo tenerla e quindi bisogna usare il codice sotto, levando l'array come struttura dati
    // (dovremo probabilmente creare una lista?).

    // Per adesso fa segm. fault
    /*
    kill(0, SIGUSR1);
    char tmp[512];
    char *pipe_str;
    char** vars;

    int i;
    for (i = 0; i < MAX_CHILDREN; i++) {
        if (children_pids[i] != -1) {
            pipe_str = pipename(children_pids[i]);
            int fd = open(pipe_str, O_RDONLY);
            if (fd > 0) {
                read(fd, tmp, 512);
                vars = split(tmp, 3);
                if (atoi(vars[2]) == in) printf("%s\n", vars[3]);
            }
        }
    }

    sleep(3);
*/
    if (in >= MAX_CHILDREN || in < 0) return -1;
    return children_pids[in] == -1 ? -1 : children_pids[in];
}

void list(char buf[][MAX_BUF_SIZE], int *children_pids) {
    kill(0, SIGUSR1);
    char tmp[512];
    char *pipe_str;
    char **vars;

    int i;
    for (i = 0; i < MAX_CHILDREN; i++) {
        if (children_pids[i] != -1) {
            pipe_str = pipename(children_pids[i]);
            int fd = open(pipe_str, O_RDONLY);
            if (fd > 0) {
                read(fd, tmp, 512);
                printf("%s\n", tmp);
            }
        }
    }
}

void info(char buf[][MAX_BUF_SIZE], int *children_pids) {
    int pid = get_by_index(atoi(buf[1]), children_pids);

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
        vars = split(tmp, 5);
        // parametri: tipo, pid, stato, tempo di accensione, indice

        printf("Oggetto: Lampadina\nPID: %s\nIndice: %s\nStato: %s\nTempo di accensione: %s\n",
               vars[1], vars[2], atoi(vars[3]) ? "ON" : "OFF", vars[4]);
    } else if (strncmp(tmp, "2", 1) == 0) {  // Frigo
        vars = split(tmp, 9);
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
        vars = split(tmp, 5);
        // parametri: tipo, pid, stato, tempo di accensione, indice
        printf("Oggetto: Finestra\nPID: %s\nIndice: %s\nStato: %s\nTempo di apertura: %s sec\n",
               vars[1], vars[2], atoi(vars[3]) ? "Aperto" : "Chiuso", vars[4]);
    } else {
        printf("Dispositivo non supportato.\n");
    }
    free(vars);
}

void __switch(char buf[][MAX_BUF_SIZE], int *children_pids) {
    int pid = get_by_index(atoi(buf[1]), children_pids);

    if (pid == -1) {
        printf("Errore! Non esiste questo dispositivo.\n");
        return;
    }

    char *pipe_str = pipename(pid);  // Nome della pipe
    char tmp[MAX_BUF_SIZE];          // dove ci piazzo l'output della pipe
    char **vars = NULL;              // output della pipe, opportunamente splittato da split()
    char pipe_message[32];           // buffer per la pipe

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
            vars = split(tmp, 5);  // parametri: tipo, pid, indice, stato, tempo di accensione,
            int status = atoi(vars[3]);
            sprintf(pipe_message, "0|0");

            if (strcmp(buf[3], "on") == 0 && status == 0) {
                write(fd, pipe_message, sizeof(pipe_message));
                kill(pid, SIGUSR2);
                printf("Lampadina accesa.\n");
            } else if (strcmp(buf[3], "off") == 0 && status == 1) {
                write(fd, pipe_message, sizeof(pipe_message));
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
            vars = split(tmp, 8);  // parametri: tipo, pid, indice, stato, tempo di accensione
            int status = atoi(vars[3]);
            sprintf(pipe_message, "0|0");

            if (strcmp(buf[3], "on") == 0 && status == 0) {
                write(fd, pipe_message, sizeof(pipe_message));
                kill(pid, SIGUSR2);
                printf("Frigorifero aperto.\n");
            } else if (strcmp(buf[3], "off") == 0 && status == 1) {
                write(fd, pipe_message, sizeof(pipe_message));
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

            write(fd, pipe_message, sizeof(pipe_message));
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

        vars = split(tmp, 5);  // parametri: tipo, pid, indice, stato, tempo di accensione
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

void add(char buf[][MAX_BUF_SIZE], int *device_i, int *children_pids) {
    if (strcmp(buf[1], "bulb") == 0 || strcmp(buf[1], "fridge") == 0 || strcmp(buf[1], "window") == 0) {
        // Aumento l'indice progressivo dei dispositivi
        (*device_i)++;

        pid_t pid = fork();
        if (pid == 0) {  // Figlio
            // Apro una pipe per padre-figlio
            char *pipe_str = pipename(getpid());
            mkfifo(pipe_str, 0666);

            // Conversione a stringa dell'indice
            char *index_str = malloc(4 * sizeof(char));
            sprintf(index_str, "%d", *device_i);

            char program_name[MAX_BUF_SIZE / 4];
            sprintf(program_name, "./%s%s", "bin/", buf[1]);

            // Metto gli argomenti in un array e faccio exec
            char *const args[] = {program_name, index_str, pipe_str, NULL};
            execvp(args[0], args);

            exit(0);
        } else {  // Padre
            printf("Aggiunta una %s con PID %i e indice %i\n", buf[1], pid, *device_i);
            children_pids[*device_i] = pid;
            return;
        }
    } else {
        printf("Dispositivo non ancora supportato\n");
    }
}

void cleanup_sig(int sig) {
    printf("Chiusura della centralina in corso...\n");
    kill(0, 9);
}
/*
void ignore_sig(int sig) {
    signal(sig, SIG_IGN);
    printf("Ignoro il segnale?\n");
}*/
/*
void handle_sig(int signal) {
    printf("Spegnimento della centralina solo tramite Launcher, Premere invio per proseguire\n");
}
*/