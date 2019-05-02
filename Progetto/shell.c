#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

char *getUserName() {
    int uid = geteuid();
    struct passwd *pw = getpwuid(uid);
    if (pw) {
        char hostname[1024];
        hostname[1023] = '\0';
        gethostname(hostname, 1023);
        return strncat(strncat(pw->pw_name, "@", 1023), hostname, 1023);
    }
    return "host";
}

int main(int argc, char *argv[]) {
    char(*buf)[128] = malloc(128 * sizeof(char *));  //Array che conterrà i comandi da eseguire
    char c, i;
    int cur_cm;
    int index = 0;
    int children[4000];  // bisogna trovare un modo per contenere i figli....questo array è temporaneo.
    // ad esempio se una lampadina ha pid 450 e indice 1 children[1] = 450

    system("clear");
    char *name = getUserName();

    while (1) {
        printf("\e[92m%s\e[39m:\e[34mCentralina\033[0m$ ", name);
        c = ' ';
        i = -1;
        cur_cm = 0;
        while (c != EOF && c != '\n') {
            c = getchar();
            if (c == ' ') {
                buf[cur_cm++][++i] = '\0';
                i = -1;
            } else {
                buf[cur_cm][++i] = c;
            }
        }
        buf[cur_cm][i] = '\0';

        //   for (int k = cur_cm; k >= 0; k--) {
        //       printf(buf[k]);
        //   }

        if (strcmp(buf[0], "exit") == 0) {  // supponiamo che l'utente scriva solo "exit" per uscire
            break;
        } else if (strcmp(buf[0], "\0") == 0) {  //a capo a vuoto
            continue;
        } else if (strcmp(buf[0], "help") == 0) {  // guida
            printf("Comandi disponibili:\n    list                        elenca tutti i dispositivi (quelli \n                                disponibili con un nome, quelli attivi\n                                anche con un “id” univoco per ciascuno \n                                e inoltre ne riepiloga le caratteristiche)\n    add <device>                aggiunge un <device> al sistema e ne\n                                mostra i dettagli (es. “add bulb”)\n    del <id>                    rimuove il dispositivo <id>: se è di controllo\n                                rimuove anche i dispositivi sottostanti\n    link <id> to <id>            collega i due dispositivi tra loro (almeno uno\n                                dei due dev’essere di controllo: controller, hub o timer)\n    switch <id> <label> <pos>    del dispositivo <id> modifica l’interruttore\n                                <label> in posizione <pos>, ad esempio:\n                                \"switch 3 open on\" imposta per il dispositivo 3\n                                 l’interruttore “open” su “on” (ad esempio apre una finestra)\n    info <id>                    mostra i dettagli del dispositivo\n");
        } else if (strcmp(buf[0], "list") == 0) {
            printf("Info su list\n");
        } else if (strcmp(buf[0], "info") == 0) {
            // Per adesso fa ben poco...bisogna aggiungere tutte le guardie
            // Prende in ingresso l'intero (NON IL PID) e restituisce il pid dell'oggetto
            if (cur_cm != 1) {
                printf("Sintassi: info <device>");
            } else {
                char *pipe_str = malloc(4 * sizeof(char));
                sprintf(pipe_str, "/tmp/%i", atoi(buf[1]));

                char tmp[128];  // dove ci piazzo l'output della pipe

                if (kill(children[atoi(buf[1])], SIGUSR1) != 0) {
                    printf("Errore! Sistema: codice errore %i\n", errno);
                    continue;
                }

                int fd = open(pipe_str, O_RDONLY);
                read(fd, tmp, 128);
                printf("Il PID della risorsa richiesta è %s\n", tmp);
                close(fd);
            }
        } else if (strcmp(buf[0], "add") == 0) {
            if (cur_cm != 1) {
                printf("Sintassi: add <device>\nDispositivi disponibili: bulb, window, fridge, hub, timer\n");
            } else {
                if (strcmp(buf[1], "bulb") == 0) {
                    // Aumento l'indice progressivo dei dispositivi
                    index++;

                    // conversione intero dell'indice a stringa....
                    char *index_str = malloc(4 * sizeof(char));
                    char *pipe_str = malloc(4 * sizeof(char));
                    sprintf(index_str, "%d", index);
                    sprintf(pipe_str, "/tmp/%i", index);

                    // Inizializzo una pipe
                    mkfifo(pipe_str, 0666);

                    //  printf(index_str);
                    //   printf(pipe_str);

                    // fflush(stdout);

                    pid_t pid = fork();
                    if (pid == 0) {  // Figlio
                        // Metto gli argomenti in un array e faccio exec
                        char *const args[] = {"./bulb", index_str, pipe_str, NULL};
                        execvp(args[0], args);

                        // Sta roba serve?
                        free(index_str);
                        exit(0);
                    } else {  // Padre
                        printf("Aggiunta una lampadina con PID %i e indice %i\n", pid, index);
                        children[index] = pid;
                        continue;
                    }
                } else {
                    printf("Codice da implementare\n");
                }
            }
        } else {  //tutto il resto
            printf("Comando non riconosciuto. Usa help per visualizzare i comandi disponibili\n");
        }
    }
    free(buf);
    return 0;
}