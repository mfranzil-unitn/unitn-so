#include "../util.h"

/* Window = 3 */

/*
Suppongo che i due interruttori che comandano la finestra Interr_Aperto e
 Interr_Chiuso siano sempre opposti. Quando ne premo uno questo resta in uno
  stato OFF (non può più essere premuto) e l'altro cambia nello stato ON (può essere premuto).

Con il comando switch permetto solo: switch <id> <Interr_Aperto/Chiuso> ON   	
*/

int shellpid; /* pid della centralina */

/* Registri della finestra */
int fd;                  /* file descriptor della pipe verso il padre */
int pid, __index, delay; /* variabili di stato */
int status = 0;          /* interruttore apertura */
time_t start, time_on;   /* tempo_accensione */
key_t key;
int msgid;

volatile int flag_usr1 = 0;
volatile int flag_usr2 = 0;
volatile int flag_term = 0;
volatile int flag_int  = 0;

/* handler dei segnali */
void sighandler_int(int sig) {
    if (sig == SIGUSR1) {
        flag_usr1 = 1;
    }
    if (sig == SIGUSR2) {
        flag_usr2 = 1;
    }
    if (sig == SIGTERM) {
        flag_term = 1;
    }
    if(sig == SIGINT){
        flag_int = 1;
    }
}

int main(int argc, char* argv[]) {
    char tmp[MAX_BUF_SIZE]; /* Buffer per le pipe*/
    char* this_pipe = NULL; /* nome della pipe */
    char** vars = NULL;     /* parametri del device */

    this_pipe = argv[2];
    pid = getpid();                 /* ottiene il pid */
    __index = atoi(argv[1]);        /* trasforma indice */
    fd = open(this_pipe, O_RDWR);   /* apre pipe */
    shellpid = get_shell_pid();     /* prende pid della shell */

    /* preparo message queue */
    key = ftok("/tmp/ipc/mqueues", pid);    /* converto pathname */
    msgid = msgget(key, 0666 | IPC_CREAT);  /* associo message queue */

    signal(SIGTERM, sighandler_int);
    signal(SIGUSR1, sighandler_int);
    signal(SIGUSR2, sighandler_int);

    while (1) {
        /* quando riceve un segnale SIGUSR1 -> invia le proprie informazioni */
        if (flag_usr1) {
            flag_usr1 = 0;

            if (status) {
                time_on = (time(NULL) - start);
            } else {
                time_on = 0;
            }

            /* compongo messaggio */
            sprintf(tmp, "3|%d|%d|%d|%d",
                    pid, __index, status, (int)time_on);
            message.mesg_type = 1;
            sprintf(message.mesg_text, "%s", tmp);
            msgsnd(msgid, &message, sizeof(message), 0); /* invio messaggio */
        }
        /* quando ricevo un segnale SIGUSR2 -> ricevo dalla message queue cosa devo fare */
        /* 0|... -> chiudi/apri finestra */
        if (flag_usr2) {
            int ret;
            flag_usr2 = 0;
            
            ret = msgrcv(msgid, &message, sizeof(message), 1, IPC_NOWAIT);
            if(ret == -1){
                printf("Errore lettura messaggio\n");
            }
            sprintf(tmp, "%s", message.mesg_text);
            vars = split_fixed(tmp, 2);

            if (atoi(vars[0]) == 0) {
                if (!status) {
                    status = 1;
                    start = time(NULL);
                } else {
                    status = 0;
                    start = 0;
                }
            }
        }
        /* quando ricevo un segnale SIGTERM -> disalloca message queue */
        if (flag_term) {
            msgctl(msgid, IPC_RMID, NULL);
            exit(0);
        }
        /* quando ricevo un segnale SIGINT -> avverto mio padre che sto per morire */
         if (flag_int) {
            int ppid;
            flag_int = 0;
            
            ppid = (int)getppid(); /* pid di mio padre */
            if (ppid != shellpid) {
                key_t key_ppid = ftok("/tmp/ipc/mqueues", ppid);
                int msgid_ppid = msgget(key_ppid, 0666 | IPC_CREAT);
                sprintf(message.mesg_text, "2|%d", pid);
                message.mesg_type = 1;
                msgsnd(msgid_ppid, &message, sizeof(message), 0);
            }
            msgctl(msgid, IPC_RMID, NULL); /* disalloco message queue */
            exit(0);
        }
    }
    return 0;
}
