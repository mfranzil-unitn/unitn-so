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
#include "util.h"

pid_t shell_pid = -1;

#define HELP_STRING_LAUNCHER \
    "Comandi disponibili:\n\
    user turn shell <on/off>\n\
    user switch <id> <label> <pos>    del dispositivo <id> modifica l’interruttore\n\
                                      <label> in posizione <pos>, ad esempio:\n\
                                      \"switch 3 open on\" imposta per il dispositivo 3\n\
                                      l’interruttore “open” su “on” (ad esempio apre una finestra)\n\
    info <id>                         mostra i dettagli del dispositivo\n"

int main(int argc, char *argv[]) {
    char(*buf)[128] = malloc(128 * sizeof(char *));
    char c, i;
    int cur_cm;
    system("clear");
    char *name = getUserName();

    while (1) {
        printf("\e[92m%s\e[39m:\e[34mLauncher\033[0m$ ", name);
        char c, i;
        int cur_cm;
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

        if (strcmp(buf[0], "exit") == 0) {  // supponiamo che l'utente scriva solo "exit" per uscire
            if (shell_pid != -1) {
                kill(shell_pid, SIGTERM);
            }
            break;
        } else if (strcmp(buf[0], "\0") == 0) {  //a capo a vuoto
            continue;
        } else if (strcmp(buf[0], "help") == 0) {  // guida
            printf(HELP_STRING_LAUNCHER);
        } else if (strcmp(buf[0], "user") == 0) {
            if (strcmp(buf[1], "turn") == 0 && strcmp(buf[2], "shell") == 0) {
                if (cur_cm != 3) {
                    printf("Sintassi: user turn shell <pos>\n");
                } else {
                    if (strcmp(buf[3], "on") == 0 && shell_pid == -1) {
                        pid_t pid = fork();
                        if (pid == 0) {
                            int ppid = (int)getppid();
                            char tmp[50] = "./bin/shell ";
                            char stringpid[6];
                            sprintf(stringpid, "%d", ppid);
                            strcat(tmp, stringpid);
                            if (execl("/usr/bin/xterm", "xterm", "-e", tmp, NULL) == -1) {
                                printf("Errore nell'apertura della centralina");
                            }
                        } else if (pid > 0) {
                            //Legge il pid della centralina dalla pipe
                            char *shpm = "/tmp/myshpm";
                            int fd = open(shpm, O_RDONLY);
                            char tmp[16];
                            read(fd, tmp, 16);
                            shell_pid = atoi(tmp);
                            close(fd);
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
            } else if (strcmp(buf[1], "switch") == 0) {
                if (cur_cm != 4) {
                    printf("Sintassi: user switch <id> <label> <pos>\nInterruttori disponibili:\n    bulb: accensione\n");
                } else {
                    printf("Codice da sistemare per l'interazione utente\n"); /*
                    // INIZIO CODICE DUPLICATO

                    char *pipe_str = malloc(4 * sizeof(char));
                    sprintf(pipe_str, "/tmp/%i", atoi(buf[1]));

                    char tmp[1024];  // dove ci piazzo l'output della pipe

                    int pid = children[atoi(buf[1])];

                    // apertura della pipe fallita
                    if (kill(pid, SIGUSR1) != 0) {
                        printf("Errore! Sistema: codice errore %i\n", errno);
                        continue;
                    }

                    int fd = open(pipe_str, O_RDONLY);
                    read(fd, tmp, 1024);

                    if (strncmp(tmp, "1", 1) == 0) {  // Lampadina
                        if (strcmp(buf[2], "accensione") != 0) {
                            printf("Operazione non permessa su una lampadina!");
                            continue;
                        }

                        char **vars = split(tmp, 5);  // parametri: tipo, stato, tempo di accensione, pid, indice
                        int status = atoi(vars[1]);
                        if (strcmp(buf[4], "on") == 0 && status == 0) {
                            kill(pid, SIGUSR2);
                            printf("Lampadina accesa.\n");
                        } else if (strcmp(buf[4], "off") == 0 && status == 1) {
                            kill(pid, SIGUSR2);
                            printf("Lampadina spenta.\n");
                        } else if (strcmp(buf[4], "off") == 0 && status == 0) {  // Spengo una lampadina spenta
                            printf("Stai provando a spegnere una lampadina spenta\n");
                        } else if (strcmp(buf[4], "on") == 0 && status == 1) {  // Spengo una lampadina accesa
                            printf("Stai provando a accendere una lampadina accesa\n");
                        } else {
                            printf("Operazione non permessa.\n");
                        }

                        free(vars);

                    } else {
                        printf("Da implementare");
                    }
                }*/
                }
            } else {
                printf("COMANDO SCONOSCIUTO\n");
            }
        } else if (strcmp(buf[0], "restart") == 0) {
            char *const args[] = {NULL};
            int pid = fork();
            if (pid == 0) {
                execvp("make", args);
                exit(0);
            } else {
                wait(NULL);
                execvp("./launcher", args);
            }
            exit(0);
        } else {  //tutto il resto
            printf("Comando non riconosciuto. Usa help per visualizzare i comandi disponibili\n");
        }
    }

    free(buf);
    return 0;
}
