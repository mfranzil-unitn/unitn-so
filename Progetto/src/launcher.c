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
#include "shell.h"
#include <sys/ipc.h> 
#include <sys/msg.h> 

pid_t shell_pid = -1;

#define HELP_STRING_LAUNCHER \
    "Comandi disponibili:\n\
    user turn shell <on/off>\n\
    user switch <id> <label> <pos>    del dispositivo <id> modifica l’interruttore\n\
                                      <label> in posizione <pos>, ad esempio:\n\
                                      \"switch 3 open on\" imposta per il dispositivo 3\n\
                                      l’interruttore “open” su “on” (ad esempio apre una finestra)\n\
    info <id>                         mostra i dettagli del dispositivo\n"    

// structure for message queue 
struct mesg_buffer { 
    long mesg_type; 
    char mesg_text[1024]; 
} message; 


int n_devices = 0;
int emergencyid;

void handle_sig(int signal){
	system("clear");
	printf("La centralina è stata chiusa, Premere Invio per proseguire\n");
	shell_pid = -1;
	}

void handle_sigint(int signal){
	// to destroy the message queue 
	msgctl(emergencyid, IPC_RMID, NULL);
	if(shell_pid =! -1){
	kill(shell_pid,SIGTERM);
	}
	}
void handle_sighup(int signal){
	system("clear");
	printf("La centralina è stata chiusa, Premere Invio per proseguire\n");
	if(shell_pid =! -1){
	kill(shell_pid,SIGTERM);
	}
	exit(0);
	}

int main(int argc, char *argv[]) {
	signal(SIGTERM, handle_sig);
	signal(SIGHUP, handle_sighup);
	signal(SIGINT, handle_sigint);
	
    char(*buf)[MAX_BUF_SIZE] = malloc(MAX_BUF_SIZE * sizeof(char *));  // array che conterrà i comandi da eseguire
    char ch;   // carattere usato per la lettura dei comandi
    int ch_i;  // indice del carattere corrente

    int cmd_n;  // numero di comandi disponibili

    int device_pids[MAX_CHILDREN];  // array contenenti i PID dei figli
    int j;
    
    for (j = 0; j < MAX_CHILDREN; j++) {
        device_pids[j] = -1;  // se è -1 non contiene nulla
    }

    
    system("clear");
    //Creo PIPE verso shell.
	char *shpm = "/tmp/myshpm";
	mkfifo(shpm,0666);
    char *name = getUserName();
    //CREO MESSAGE QUEUE TRA SHELL E LAUNCHERRRRRRRRRRRRRRR
	key_t key;
	key = ftok("progfile",65);
	int msgid;
	msgid = msgget(key, 0666 | IPC_CREAT); 	
	emergencyid = msgid;
        while (1) {
		//Leggo il numero di devices presenti.
		if(shell_pid >0){
			int ret = msgrcv(msgid, &message, sizeof(message), 1, IPC_NOWAIT); 
			if(ret != -1){
			int q=0;
			char n_dev_str[100];
			while(!(message.mesg_text[q] == '|')){
				n_dev_str[q] = message.mesg_text[q];
				q++;
				}
				n_dev_str[q] = '\0';
			n_devices = atoi(n_dev_str);
			int __count = n_devices;
			char tmp_buf[1024];
			sprintf(tmp_buf,"%s", message.mesg_text);
			char *tokenizer = strtok(tmp_buf, "|");
			char **vars = malloc(__count * sizeof(char *));
			int j = 0;
			while (tokenizer != NULL && j <= __count) {
				vars[j++] = tokenizer;
				tokenizer = strtok(NULL, "|");
				if(j >=2){
					device_pids[j-1] = atoi(vars[j-1]);
				}
			}
		}
		}else{
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
        while (ch != EOF && ch != '\n'){
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
        } 
         else if (strcmp(buf[0], "info") == 0) {  // info su dispositivo
			 if(cmd_n != 1){
				 printf("Sintassi: info <device>\n");
				 }
			////CODICE DUPLICATOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO
			else{
				if(shell_pid >0){
			int ret = msgrcv(msgid, &message, sizeof(message), 1, IPC_NOWAIT); 
			if(ret != -1){
			int q=0;
			char n_dev_str[100];
			while(!(message.mesg_text[q] == '|')){
				n_dev_str[q] = message.mesg_text[q];
				q++;
				}
				n_dev_str[q] = '\0';
			n_devices = atoi(n_dev_str);
			int __count = n_devices;
			char tmp_buf[1024];
			sprintf(tmp_buf,"%s", message.mesg_text);
			char *tokenizer = strtok(tmp_buf, "|");
			char **vars = malloc(__count * sizeof(char *));
			int j = 0;
			while (tokenizer != NULL && j <= __count) {
				vars[j++] = tokenizer;
				tokenizer = strtok(NULL, "|");
				if(j >=2){
					device_pids[j-1] = atoi(vars[j-1]);
				}
			}
		}
		info(buf, device_pids);
		}
		else{
			printf("La centralina è spenta\n");
			}
        }
        
        }else if (strcmp(buf[0], "user") == 0) { //I comandi da launcher fuorchè per help ed exit devono cominciare per user.
            if (strcmp(buf[1], "turn") == 0 && strcmp(buf[2], "shell") == 0) {
                if (cmd_n != 3) { //Controllo correttezza nel conteggio degli argomenti.
                    printf("Sintassi: user turn shell <pos>\n");
                } else {
                    if (strcmp(buf[3], "on") == 0 && shell_pid == -1) { //Se non è ancora accesa => shell_pid == -1
                        pid_t pid = fork();
                        if(pid < 0){
				printf("Errore in fork\n");
				exit(1);
			}
                        if (pid == 0) { //Processo figlio che aprirà terminale e lancerà la shell.
							
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
                                write(fd,tmp,16);
                                close(fd);
                            }
                        } else if (pid > 0) {
                            //Legge il contenuto della pipe => Se = "Errore" la finestra è stata aperta.
			int fd = open(shpm, O_RDONLY);
                            char tmp[16];
                            read(fd, tmp, 16);
                            if(strcmp(tmp, "Errore") == 0){
				printf("Errore nell'apertura della Shell\n");
				}
			else{
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
					//CODICE DUPLICATO
					if(shell_pid >0){
			int ret = msgrcv(msgid, &message, sizeof(message), 1, IPC_NOWAIT); 
			if(ret != -1){
			int q=0;
			char n_dev_str[100];
			while(!(message.mesg_text[q] == '|')){
				n_dev_str[q] = message.mesg_text[q];
				q++;
				}
				n_dev_str[q] = '\0';
			n_devices = atoi(n_dev_str);
			int __count = n_devices;
			char tmp_buf[1024];
			sprintf(tmp_buf,"%s", message.mesg_text);
			char *tokenizer = strtok(tmp_buf, "|");
			char **vars = malloc(__count * sizeof(char *));
			int j = 0;
			while (tokenizer != NULL && j <= __count) {
				vars[j++] = tokenizer;
				tokenizer = strtok(NULL, "|");
				if(j >=2){
					device_pids[j-1] = atoi(vars[j-1]);
				}
			}
		}
		}
					if(shell_pid > 0){
						if(atoi(buf[1]) <= n_devices){
							__switch(buf,device_pids);
							}
						else{
							printf("Id non presente\n");
							}
						
						}else{printf("Azione non disponibile a centralina spenta\n");}
                }
            }
        else if (strcmp(buf[0], "restart") == 0) {
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
    
	// to destroy the message queue 
	msgctl(msgid, IPC_RMID, NULL); 	
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
                vars = sp-lit(tmp, 3);
                if (atoi(vars[2]) == in) printf("%s\n", vars[3]);
            }
        }
    }

    sleep(3);
*/
    if (in >= MAX_CHILDREN || in < 0) return -1;
    return children_pids[in] == -1 ? -1 : children_pids[in];
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
        vars = split(tmp);
        // parametri: tipo, pid, stato, tempo di accensione, indice

        printf("Oggetto: Lampadina\nPID: %s\nIndice: %s\nStato: %s\nTempo di accensione: %s\n",
               vars[1], vars[2], atoi(vars[3]) ? "ON" : "OFF", vars[4]);
    } else if (strncmp(tmp, "2", 1) == 0) {  // Frigo
        vars = split(tmp);
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
        vars = split(tmp);
        // parametri: tipo, pid, stato, tempo di accensione, indice
        printf("Oggetto: Finestra\nPID: %s\nIndice: %s\nStato: %s\nTempo di apertura: %s sec\n",
               vars[1], vars[2], atoi(vars[3]) ? "Aperto" : "Chiuso", vars[4]);
    } else {
        printf("Dispositivo non supportato.\n");
    }
    free(vars);
}


void __switch(char buf[][MAX_BUF_SIZE], int *children_pids ) {
    int pid = get_by_index(atoi(buf[1]), children_pids);
    if (pid == -1) {
        printf("Errore! Non esiste questo dispositivo.\n");
        return;
    }

    char *pipe_str = pipename(pid);  // Nome della pipe
    char tmp[MAX_BUF_SIZE];          // dove ci piazzo l'output della pipe
    char **vars = NULL;              // output della pipe, opportunamente diviso
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
            vars = split(tmp);  // parametri: tipo, pid, indice, stato, tempo di accensione,
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
            vars = split(tmp);  // parametri: tipo, pid, indice, stato, tempo di accensione
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

        vars = split(tmp);  // parametri: tipo, pid, indice, stato, tempo di accensione
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
