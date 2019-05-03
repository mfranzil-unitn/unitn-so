#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

const int MAX_BUF_SIZE = 1024;
const int MAX_CHILDREN = 100;

int children_pids[100];  // array contenenti i PID dei figli

int device_i = 0;  // indice progressivo dei dispositivi

// CENTRALINA = 0
// BULB = 1
// FRIGO = 2

char *getUserName() {
    int uid = geteuid();
    struct passwd *pw = getpwuid(uid);
    if (pw) {
        char hostname[MAX_BUF_SIZE];
        hostname[1023] = '\0';
        gethostname(hostname, 1023);
        return strncat(strncat(pw->pw_name, "@", 1023), hostname, 1023);
    }
    return "host";
}

char **split(char *tmp, int count) {
    char *tokenizer = strtok(tmp, "|");
    char **vars = malloc(count * sizeof(char *));
    int j = 0;

    while (tokenizer != NULL) {
        vars[j++] = tokenizer;
        tokenizer = strtok(NULL, "|");
    }

    return vars;
}

char *pipename(int pid) {
    char *pipe_str = malloc(4 * sizeof(char));
    sprintf(pipe_str, "/tmp/%i", pid);
    return pipe_str;
}

int get_by_index(int in) {
    if (in >= MAX_CHILDREN || in < 0) return -1;
    return children_pids[in] == -1 ? -1 : children_pids[in];
}

void add(char buf[][MAX_BUF_SIZE]) {
    //char *localized_name;

    if (strcmp(buf[1], "bulb") == 0 || strcmp(buf[1], "fridge") == 0 || strcmp(buf[1], "window") == 0) {
        // Aumento l'indice progressivo dei dispositivi
        device_i++;

        pid_t pid = fork();
        if (pid == 0) {  // Figlio
            // Apro una pipe per padre-figlio
            char *pipe_str = pipename(getpid());
            mkfifo(pipe_str, 0666);

            // Conversione a stringa dell'indice
            char *index_str = malloc(4 * sizeof(char));
            sprintf(index_str, "%d", device_i);

            char program_name[64];
            sprintf(program_name, "./%s%s", "", buf[1]);

            // Metto gli argomenti in un array e faccio exec
            char *const args[] = {program_name, index_str, pipe_str, NULL};
            execvp(args[0], args);

            exit(0);
        } else {  // Padre
            printf("Aggiunta una %s con PID %i e indice %i\n", buf[1], pid, device_i);
            children_pids[device_i] = pid;
            return;
        }
    } else {
        printf("Dispositivo non ancora supportato\n");
    }
}

int main(int argc, char *argv[]) {
    char(*buf)[MAX_BUF_SIZE] = malloc(MAX_BUF_SIZE * sizeof(char *));  // array che conterrà i comandi da eseguire

    char ch;   // carattere usato per la lettura dei comandi
    int ch_i;  // indice del carattere corrente

    int cmd_n;  // numero di comandi disponibili

    int j;
    for (j = 0; j < MAX_CHILDREN; j++) {
        children_pids[j] = -1;  // se è -1 non contiene nulla
    }

    system("clear");
    char *name = getUserName();

    while (1) {
        printf("\e[92m%s\e[39m:\e[34mCentralina\033[0m$ ", name);

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
            continue;
        } else if (strcmp(buf[0], "help") == 0) {  // guida
            printf("Comandi disponibili:\n    list                        elenca tutti i dispositivi (quelli \n                                disponibili con un nome, quelli attivi\n                                anche con un “id” univoco per ciascuno \n                                e inoltre ne riepiloga le caratteristiche)\n    add <device>                aggiunge un <device> al sistema e ne\n                                mostra i dettagli (es. “add bulb”)\n    del <id>                    rimuove il dispositivo <id>: se è di controllo\n                                rimuove anche i dispositivi sottostanti\n    link <id> to <id>            collega i due dispositivi tra loro (almeno uno\n                                dei due dev’essere di controllo: controller, hub o timer)\n    switch <id> <label> <pos>    del dispositivo <id> modifica l’interruttore\n                                <label> in posizione <pos>, ad esempio:\n                                \"switch 3 open on\" imposta per il dispositivo 3\n                                 l’interruttore “open” su “on” (ad esempio apre una finestra)\n    info <id>                    mostra i dettagli del dispositivo\n");
        } else if (strcmp(buf[0], "list") == 0) {
            printf("Info su list\n");
        } else if (strcmp(buf[0], "info") == 0) {
            // Per adesso fa ben poco...bisogna aggiungere tutte le guardie
            // Prende in ingresso l'intero (NON IL PID) e restituisce il pid dell'oggetto
            if (cmd_n != 1) {
                printf("Sintassi: info <device>");
            } else {
                int pid = get_by_index(atoi(buf[1]));

                if (pid == -1) {
                    printf("Errore! Non esiste questo dispositivo.");
                    continue;
                }

                char *pipe_str = pipename(pid);
                char tmp[MAX_BUF_SIZE];  // dove ci piazzo l'output della pipe

                // apertura della pipe fallita
                if (kill(pid, SIGUSR1) != 0) {
                    printf("Errore! Sistema: codice errore %i\n", errno);
                    continue;
                }

                int fd = open(pipe_str, O_RDONLY);
                read(fd, tmp, MAX_BUF_SIZE);

                if (strncmp(tmp, "1", 1) == 0) {  // Lampadina
                    char **vars = split(tmp, 5);
                    // parametri: tipo, stato, tempo di accensione, pid, indice

                    printf("Oggetto: Lampadina\nStato: %s\nTempo di accensione: %s\nPID: %s\nIndice: %s\n",
                           atoi(vars[1]) ? "ON" : "OFF", vars[2], vars[3], vars[4]);
                    free(vars);
                } else if (strncmp(tmp, "2", 1) == 0) {  // Frigo
                    char **vars = split(tmp, 9);
                    // parametri: tipo, stato, tempo di apertura, pid, indice, delay
                    // percentuale riempimento, temperatura interna

                    printf("Oggetto: Frigorifero\n");

                    if (vars[8] != NULL) {
                        printf("[!!] Messaggio di log: <%s>\n", vars[8]);
                    }
                    printf("Stato: %s\nTempo di apertura: %s sec\nPID: %s\nIndice: %s\n",
                           atoi(vars[1]) ? "ON" : "OFF", vars[2], vars[3], vars[4]);
                    printf("Delay richiusura: %s sec\nPercentuale riempimento: %s\nTemperatura: %s°C\n",
                           vars[5], vars[6], vars[7]);
                    free(vars);
                } else if (strncmp(tmp, "3", 1) == 0) {  // Finestra
                    char **vars = split(tmp, 5);

                    // parametri: tipo, stato, tempo di accensione, pid, indice
                    printf("Oggetto: Finestra\nStato: %s\nTempo di apertura: %s sec\nPID: %s\nIndice: %s\n",
                           atoi(vars[1]) ? "ON" : "OFF", vars[2], vars[3], vars[4]);
                    free(vars);
                } else {
                    printf("Dispositivo non supportato.\n");
                }

                free(pipe_str);
                close(fd);
            }
        } else if (strcmp(buf[0], "switch") == 0) {
            if (cmd_n != 3) {
                printf("Sintassi: switch <id> <label> <pos>\nInterruttori disponibili:\n    bulb: accensione\n    fridge: apertura\n");
            } else {
                // INIZIO CODICE DUPLICATO
                int pid = get_by_index(atoi(buf[1]));

                if (pid == -1) {
                    printf("Errore! Non esiste questo dispositivo.\n");
                    continue;
                }

                char *pipe_str = pipename(pid);
                char tmp[MAX_BUF_SIZE];  // dove ci piazzo l'output della pipe

                // apertura della pipe fallita
                if (kill(pid, SIGUSR1) != 0) {
                    printf("Errore! Impossibile notificare il dispositivo. Errno: %i\n", errno);
                    continue;
                }

                int fd = open(pipe_str, O_RDONLY);
                read(fd, tmp, MAX_BUF_SIZE);

                if (strncmp(tmp, "1", 1) == 0) {  // Lampadina
                    if (strcmp(buf[2], "accensione") != 0) {
                        printf("Operazione non permessa su una lampadina!\nOperazioni permesse: accensione\n");
                        continue;
                    }

                    char **vars = split(tmp, 5);  // parametri: tipo, stato, tempo di accensione, pid, indice
                    int status = atoi(vars[1]);
                    if (strcmp(buf[3], "on") == 0 && status == 0) {
                        kill(pid, SIGUSR2);
                        printf("Lampadina accesa.\n");
                    } else if (strcmp(buf[3], "off") == 0 && status == 1) {
                        kill(pid, SIGUSR2);
                        printf("Lampadina spenta.\n");
                    } else if (strcmp(buf[3], "off") == 0 && status == 0) {  // Spengo una lampadina spenta
                        printf("Stai provando a spegnere una lampadina spenta!\n");
                    } else if (strcmp(buf[3], "on") == 0 && status == 1) {  // Spengo una lampadina accesa
                        printf("Stai provando a accendere una lampadina accesa!\n");
                    } else {
                        printf("Sintassi non corretta. Sintassi: switch <bulb> accensione <on/off>\n");
                    }
                    free(vars);
                } else if (strncmp(tmp, "2", 1) == 0) {  // Fridge

                    // MANCA IL TERMOSTATOOOOOOOOOOOOOOO

                    if (strcmp(buf[2], "apertura") != 0) {
                        printf("Operazione non permessa su un frigorifero!\nOperazioni permesse: apertura\n");
                        continue;
                    }

                    char **vars = split(tmp, 8);  // parametri: tipo, stato, tempo di accensione, pid, indice
                    int status = atoi(vars[1]);
                    if (strcmp(buf[3], "on") == 0 && status == 0) {
                        kill(pid, SIGUSR2);
                        printf("Frigorifero aperto.\n");
                    } else if (strcmp(buf[3], "off") == 0 && status == 1) {
                        kill(pid, SIGUSR2);
                        printf("Frigorifero chiuso.\n");
                    } else if (strcmp(buf[3], "off") == 0 && status == 0) {  // Chiudo frigo già chiuso
                        printf("Stai provando a chiudere un frigorigero già chiuso.\n");
                    } else if (strcmp(buf[3], "on") == 0 && status == 1) {  // Apro frigo già aperto
                        printf("Stai provando a aprire un frigorifero già aperto.\n");
                    } else {
                        printf("Sintassi non corretta. Sintassi: switch <fridge> apertura <on/off>\n");
                    }

                    free(vars);
                } else {  // tutti gli altri dispositivi
                    printf("Dispositivo non supportato.\n");
                }
            }
        } else if (strcmp(buf[0], "add") == 0) {
            if (cmd_n != 1) {
                printf("Sintassi: add <device>\nDispositivi disponibili: bulb, window, fridge, hub, timer\n");
            } else {
                add(buf);
            }
        } else {  //tutto il resto
            printf("Comando non riconosciuto. Usa help per visualizzare i comandi disponibili\n");
        }
    }
    free(buf);
    return 0;
}