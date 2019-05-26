#include "../actions.h"
#include "../util.h"

/* TIMER = 5 */

struct rctm {
    int hour;
    int min;
};

int shellpid;

/* Registri del timer */
int fd;                  /* file descriptor della pipe verso il padre */
int pid, __index;        /* variabili di stato */
int status = 0;          /* interruttore accensione */
int status_override = 0; /* override 2 acceso - 3 spento*/

/* Registri per il figlio - array usato per intercompatibilità */
int children_pids[1];
int override = 0;
char info[MAX_BUF_SIZE];

/* Parametri della message queue e dela message id basate su indice */
key_t key;
int msgid;

/* Parametri della message queue e dela message id basate su pid */
key_t key_pid;
int msgid_pid;

/* strutture di time.h per la gestione della data */
struct rctm tm_start;
struct rctm tm_end;

/* Flag relativi al segnale appena ricevuto */
volatile int flag_usr1 = 0;
volatile int flag_usr2 = 0;
volatile int flag_term = 0;
volatile int flag_alarm = 0;
volatile int flag_int = 0;
volatile int flag_urg = 0;

/* Funzione chiamata in chiusura
 * che scrive sull messagequeue, basata su indice
 * il figli attualmente presente */
void term();
/* Legge la messagequeue dell'indice, in apertura, e aggiunge i figli presenti nella messagequeue
 * (FONDAMENTALE PER RICOSTRUZIONE GERARCHIA) */
void read_msgqueue(int msgid);

/* Restituisce 1 solo se il figlio è in override */
int check_override() {
    int ret = 0;
    char** vars;

    if (children_pids[0] != -1) {
        char* raw_info = get_raw_device_info(children_pids[0]);

        if (raw_info == NULL) {
            return ret;
        }

        vars = split(raw_info);
        if (atoi(vars[3]) != status) {
            ret = 1;
        }
    }

    return ret;
}

/* Funzione chiamata in seguito al comando switch ed in aggiunta di qualsiasi figlio
 * per modificarne l'interrutore */
void switch_child() {
    char* raw_info;
    if (children_pids[0] == -1) {
        return;
    }

    /* Ottiene le raw_info del figlio */
    raw_info = get_raw_device_info(children_pids[0]);

    /* Chiamata alla funzione diretta in actions.c*/
    __switch(children_pids[0], "generic_on_off", status ? "on" : "off", raw_info);
}

/*
 * Funzione che setta un alarm dipedente dalla differenza dal tempo della chiamata al prossimo tempo di spegnimento.
 * Appoggiandosi a strutture di time.h
 */
void check_time() {
    /* Ottiene il tempo attuale */
    struct tm tmp = *localtime(&(time_t){time(NULL)});
    struct rctm tm_current;
    const int SECONDS_IN_A_DAY = 86400;

    tm_current.hour = tmp.tm_hour;
    tm_current.min = tmp.tm_min;

    /* Conversione in secondi */
    int tm_current_seconds = 60 * 60 * tm_current.hour + 60 * tm_current.min + tmp.tm_sec;
    int tm_end_seconds = 60 * 60 * tm_end.hour + 60 * tm_end.min;
    int tm_start_seconds = 60 * 60 * tm_start.hour + 60 * tm_start.min;

    /* Sono nella fascia oraria => Devo accendere il dispositivo */
    if (tm_current_seconds >= tm_start_seconds && tm_current_seconds < tm_end_seconds) {
        /* Sono nella "fascia oraria" */
        status = 1;
        /* Setto(alarm) */
        alarm((tm_end_seconds - tm_current_seconds) % SECONDS_IN_A_DAY);
        switch_child();
    }
    /* Dispositivo spento, non sono nella fascia oraria */
    else {
        status = 0;
        alarm((tm_start_seconds - tm_current_seconds) % SECONDS_IN_A_DAY);
        switch_child();
    }
    flag_alarm = 0;
    return;
}

/* handler che in funzione del segnale setta un flag */
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
    if (sig == SIGALRM) {
        flag_alarm = 1;
    }
    if (sig == SIGINT) {
        flag_int = 1;
    }
    if (sig == SIGURG) {
        flag_urg = 1;
    }
}

int main(int argc, char* argv[]) {
    /* argv = [./timer, indice, /tmp/indice]; */
    char tmp[MAX_BUF_SIZE];
    char* this_pipe = NULL; /* nome della pipe */
    int over_index[MAX_CHILDREN];
    /* Le seguenti variabili sono usate in usr2 */
    char* raw_info = NULL;
    int code;
    char** vars = NULL;

    int i;

    /* Inizializza pipe ed indice */
    this_pipe = argv[2];
    pid = getpid();
    __index = atoi(argv[1]);
    fd = open(this_pipe, O_RDWR);

    shellpid = get_shell_pid();

    tm_start.hour = 8;
    tm_start.min = 0;
    tm_end.hour = 9;
    tm_end.min = 0;

    /* Viene settato ad 1 il flag alarm */
    flag_alarm = 1;
    children_pids[0] = -1;

    /* Segnali assegnati ad handler relativi */
    signal(SIGCHLD, SIG_IGN);
    signal(SIGTERM, sighandler_int);
    signal(SIGUSR1, sighandler_int);
    signal(SIGUSR2, sighandler_int);
    signal(SIGALRM, sighandler_int);
    signal(SIGINT, sighandler_int);
    signal(SIGURG, sighandler_int);

    /* Inizializzo message queue basata su indice */
    key = ftok("/tmp/ipc/mqueues", __index);
    msgid = msgget(key, 0666 | IPC_CREAT);

    /* Legge se ha dispositivo da aggiungersi (Fondamentale per consistenza della gerarchia) */
    read_msgqueue(msgid);

    /* Inizializzo message queue basata su pid */
    key_pid = ftok("/tmp/ipc/mqueues", pid);
    msgid_pid = msgget(key_pid, 0666 | IPC_CREAT);

    while (1) {
        __index = atoi(argv[1]);
        if (flag_usr1) {
            flag_usr1 = 0;
            override = 0;
            status_override = 0;

            /* Comunico informazioni sulla message queue, all'interno concateno informazioni del dispositivo figlio, se presente. */
            sprintf(tmp, "5|%d|%d|%d|%d|%d|%d|%d|%d|<!|",
                    pid, __index, status,
                    tm_start.hour, tm_start.min,
                    tm_end.hour, tm_end.min,
                    children_pids[0] != -1);
            /* Eseguo concatenazione */
            if (children_pids[0] != -1) {
                char* raw_info = get_raw_device_info(children_pids[0]);

                if (raw_info != NULL) {
                    char raw_tmp[MAX_BUF_SIZE];
                    sprintf(raw_tmp, "%s", raw_info);
                    char** raw_split = split(raw_tmp);

                    /* Distinguo vari casi di override */
                    if (atoi(raw_split[3]) != status) {
                        override = 1;
                        if (status) {
                            status_override = 2;
                        } else {
                            status_override = 3;
                        }
                    }

                    strcat(tmp, raw_info);
                    strcat(tmp, "|!|");
                    free(raw_info);
                }
            }
            strcat(tmp, "!>");

            /* Concateno lo stato di override */
            int sep = 0;
            for (i = 0; i < 20 && sep < 3 && (status_override == 2 || status_override == 3); i++) {
                if (tmp[i] == '|') {
                    sep++;
                }
                if (sep == 3) {
                    char c = status_override + '0';
                    tmp[i + 1] = c;
                }
            }
            /* Mando messaggio sulla coda relativa al pid */
            message.mesg_type = 1;
            sprintf(message.mesg_text, "%s", tmp);
            msgsnd(msgid_pid, &message, sizeof(message), 0);
        }
        if (flag_usr2) {
            flag_usr2 = 0;
            /* 
                Al ricevimento del segnale, la finestra apre la pipe in lettura e ottiene cosa deve fare.
                0|.. -> Spegni/Accendi
                1|FIGLIO -> Aggiungi figlio
                2|PID -> Rimuovi figlio
                3|ORA|MINUTI|ORAFINE|MINUTIFINE -> imposta timer

            */

            /* Ricevo il messaggio */
            msgrcv(msgid_pid, &message, sizeof(message), 1, 0);
            sprintf(tmp, "%s", message.mesg_text);

            /* Codice è il primo carattere del messaggio */
            code = tmp[0] - '0';

            if (code == 0) {
                override = check_override(over_index);
                /* Cambio lo status */
                status = !status;
                if (children_pids[0] != -1) {
                    /* Cambio lo stato del figlio */
                    switch_child();
                }
            }
            if (code == 1) {
                /* Devo rimuovere i primi due caratteri per passare i parametri nel modo corretto */
                char* shifted_tmp = malloc(MAX_BUF_SIZE * sizeof(shifted_tmp));
                strcpy(shifted_tmp, tmp);
                shifted_tmp = shifted_tmp + 2;

                /* Splitto le variabili in modo da poter aggiungere il figlio */
                vars = split(shifted_tmp);
                /* Metodo che aggiunge il figlio */
                __add_ex(vars, children_pids, 1);

                sleep(2);
                /* Mi assicuro la congruenza, almeno iniziale degli stati */
                switch_child();
                free(vars);
                free(shifted_tmp - 2);
            }
            /*Modifica di altri interruttori*/
            if (code == 3) {
                vars = split_fixed(tmp, 5);

                tm_start.hour = atoi(vars[1]);
                tm_start.min = atoi(vars[2]);
                tm_end.hour = atoi(vars[3]);
                tm_end.min = atoi(vars[4]);

                flag_alarm = 1;
            }
        }
        /* Chiamato in caso di del del padre o di link */
        if (flag_term) {
            term();
        }
        /* Fondamentale per funzionamento timer */
        if (flag_alarm) {
            check_time();
        }
        /* Chiamato in caso di eliminazione diretta */
        if (flag_int) {
            int ppid = (int)getppid();
            /* Se padre è shell questo viene risolto immediatamente*/
            if (ppid != shellpid) {
                /* Apre coda verso il padre */
                key_t key_ppid = ftok("/tmp/ipc/mqueues", ppid);
                int msgid_ppid = msgget(key_ppid, 0666 | IPC_CREAT);
                sprintf(message.mesg_text, "2|%d", pid);
                message.mesg_type = 1;
                /* Manda messaggio ed avverte che questo è stato mandato */
                msgsnd(msgid_ppid, &message, sizeof(message), 0);
                kill(ppid, SIGURG);
            }
            if (children_pids[0] != -1) {
                /* Killa il figlio */
                kill(children_pids[0], SIGTERM);
            }
            /* Elimina la coda relativa al pid */
            msgctl(msgid_pid, IPC_RMID, NULL);
            exit(0);
        }
        /* Chiamato all'eliminazione di un figlio */
        if (flag_urg) {
            flag_urg = 0;
            char** vars;
            /* Riceve messaggio contenente il pid */
            msgrcv(msgid_pid, &message, sizeof(message), 1, 0);
            sprintf(tmp, "%s", message.mesg_text);
            vars = split(tmp);
            if (children_pids[0] == atoi(vars[1])) {
                /* setta il pid del dispositivo a -1 */
                children_pids[0] = -1;
            }
        }
    }

    return 0;
}

/* Chiamato in caso di eliminazione del padre o di linking
 * il metodo manda alla coda relativa all'indice le informazione dei figli*/

void term() {
    int i;
    char tmp[MAX_BUF_SIZE - sizeof(int)]; /* POI VA CONCATENATO */

    int count = 0;
    char intern[MAX_BUF_SIZE];
    char* info;

    sprintf(tmp, "-");
    /* Se figlio presente => Richide le informazioni */
    if (children_pids[0] != -1) {
        /* Richiede le informazioni al figlio */
        char* son_info = get_raw_device_info(children_pids[0]);
        char intern[MAX_BUF_SIZE];
        sprintf(intern, "-%s", son_info);
        strcat(tmp, son_info);
        sprintf(message.mesg_text, "1%s", tmp);
    }
    /* Non fa nulla in caso contrario */
    else {
        sprintf(message.mesg_text, "0%s", tmp);
    }
    /* Manda il messaggio alla propria coda relativa all'indice */
    message.mesg_type = 1;
    msgsnd(msgid, &message, sizeof(message), 0);

    msgctl(msgid_pid, IPC_RMID, NULL);
    exit(0);
}

void read_msgqueue(int msgid) {
    int n_devices;
    int ret;
    int q, j;
    char n_dev_str[100];
    int __count;
    char tmp_buf[MAX_BUF_SIZE];
    char** vars;
    char** son_j;

    /* Legge la messagequeue relativa all'indice per controllare se vi è il figlio da aggiungere (=> ret != -1) */
    ret = msgrcv(msgid, &message, sizeof(message), 1, IPC_NOWAIT);

    if (ret != -1) {
        /* Controlla che vi sia il figlio*/
        q = 0;
        while (!(message.mesg_text[q] == '-')) {
            n_dev_str[q] = message.mesg_text[q];
            q++;
        }
        n_dev_str[q] = '\0';
        n_devices = atoi(n_dev_str);
        if (n_devices > 0) {
            /* Splitta le info relative al figlio e lo aggiunge */
            __count = n_devices;
            sprintf(tmp_buf, "%s", message.mesg_text);
            vars = NULL;
            vars = split_sons(tmp_buf, __count);
            son_j = split(vars[1]);
            __add_ex(son_j, children_pids, 1);
        }
    }
}
