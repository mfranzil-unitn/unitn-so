#include "../actions.h"
#include "../util.h"

/* HUB = 4 */

int shellpid;

int fd;                  /* file descriptor della pipe verso il padre */
int pid, __index;        /* variabili di stato */
int status = 0;          /* interruttore accensione */
int status_override = 0; /* override 2 acceso - 3 spento*/

int children_pids[MAX_CHILDREN]; /* Array contente pids dei figli */
int override = 0; /* Intero che segnala lo stato di override */
char info[MAX_BUF_SIZE];

/* Parametri della message queue e dela message id basate su indice */
key_t key;
int msgid;

/* Parametri della message queue e dela message id basate su pid */
key_t key_pid;
int msgid_pid;

/* Flag relativi al segnale appena ricevuto */
volatile int flag_usr1 = 0;
volatile int flag_usr2 = 0;
volatile int flag_term = 0;
volatile int flag_urg = 0;
volatile int flag_int = 0;

/* Funzione chiamata in chiusura
 * che scrive sull messagequeue, basata su indice
 * i figli attualmente presenti */
void term();

/* Legge la messagequeue dell'indice, in apertura, e aggiunge i figli presenti nella messagequeue
 * (FONDAMENTALE PER RICOSTRUZIONE GERARCHIA) */
void read_msgqueue(int msgid);

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
    if (sig == SIGURG) {
        flag_urg = 1;
    }
    if (sig == SIGINT) {
        flag_int = 1;
    }
}

/* Funzione chiamata in seguito al comando switch ed in aggiunta di qualsiasi figlio
 * per modificarne l'interrutore */
void switch_child(int children_index, int device_type) {
    /* In funzione del device indica l'interruttore corretto */
    char switch_names[6][MAX_BUF_SIZE] = {"-", "accensione", "apertura", "apertura", "accensione", "accensione"};

    if (children_pids[0] == -1) {
        return;
    }

    __switch_index(children_index, switch_names[device_type], status ? "on" : "off", children_pids);
}

/*Itera sui figli, in realtà fino a MAX_CHILDREN, e controlla che gli stati siano congruenti. */
/*E modifica il vettore over_index Maschera di bit. */
int check_override(int* over_index) {
    int i = 0;
    int ret = 0;
    char** vars;

    for (i = 0; i < MAX_CHILDREN; i++) {
        if (children_pids[i] != -1) {
            /* Funzione che ritorna le informazioni, ancora non splittate */
            char* raw_info = get_raw_device_info(children_pids[i]);

            if (raw_info == NULL) {
                continue;
            }

            /* Funzione che le splitta */
            vars = split(raw_info);
            if (atoi(vars[3]) != status) {
                over_index[i] = 1;
                ret = 1;
            }
        }
    }

    /* se = 1 => Override, se =0 => Non override */
    return ret;
}

int main(int argc, char* argv[]) {
    /* argv = [./hub, indice, /tmp/indice]; */
    char tmp[MAX_BUF_SIZE];
    char* this_pipe = NULL; /* nome della pipe */
    int over_index[MAX_CHILDREN];
    /* Le seguenti variabili sono usate in usr2 */
    char* raw_info = NULL;
    int code;
    char** vars;

    int i;

    int connected = 0;

    /* Viene associata la pipe relativa (passata per argomento) */
    this_pipe = argv[2];
    /* inizializza la variabile al proprio pid */
    pid = (int)getpid();
    /* inizializza indice*/
    __index = atoi(argv[1]);
    /* apre pripe in RDWR */
    fd = open(this_pipe, O_RDWR);

    /* Inizializza array dei figli */
    for (i = 0; i < MAX_CHILDREN; i++) {
        children_pids[i] = -1;
    }

    /* Inizializza key in funzione dell'indice */
    key = ftok("/tmp/ipc/mqueues", __index);
    msgid = msgget(key, 0666 | IPC_CREAT);

    read_msgqueue(msgid);

    /*Inizializza shellpid, variabile contenente il pid della variabile */
    shellpid = get_shell_pid();

    /* Inizializza MessageQueue in funzione del pid*/
    key_pid = ftok("/tmp/ipc/mqueues", pid);
    msgid_pid = msgget(key_pid, 0666 | IPC_CREAT);

    /* Assegna segnali ed handler */
    signal(SIGCHLD, SIG_IGN);
    signal(SIGTERM, sighandler_int);
    signal(SIGUSR1, sighandler_int);
    signal(SIGUSR2, sighandler_int);
    signal(SIGURG, sighandler_int);
    signal(SIGINT, sighandler_int);

    while (1) {
        __index = atoi(argv[1]);

        if (flag_usr1) {
            flag_usr1 = 0;
            override = 0;
            status_override = 0;
            /* conto i dispositivi connessi */
            connected = 0;

            /* Itero sui figli se != -1 il pid corrispondente => Sono da contare */
            for (i = 0; i < MAX_CHILDREN; i++) {
                if (children_pids[i] != -1) {
                    connected++;
                }
            }

            /* Scrive su tmp le proprie raw_info
             * pid|indice|status|numero di dipositiviconnessi <! le info di questi */
            sprintf(tmp, "4|%d|%d|%d|%d|<!|",
                    pid, __index, status, connected);

            /* Stampo nel buffer tante volte quanti device ho */
            for (i = 0; i < MAX_CHILDREN; i++) {
                if (children_pids[i] != -1) {
                    /* Prendo le raw_info del figlio con pid children_pids[i] */
                    raw_info = get_raw_device_info(children_pids[i]);
                    char raw_tmp[MAX_BUF_SIZE];
                    /* Mi assicuro che split non corrompa raw_info */
                    sprintf(raw_tmp, "%s", raw_info);
                    /* Splitto le raw_info */
                    char** raw_split = split(raw_tmp);
                    if (atoi(raw_split[3]) != status) {

                        /* Se gli status non corrispondono sono in override */
                        override = 1;

                        /* setta la "tipologia" di override */
                        if (status) {
                            status_override = 2;
                        } else {
                            status_override = 3;
                        }
                    }
                    if (raw_info != NULL) {
                        /* se non sono nulle le concateno in tmp */
                        strcat(tmp, raw_info);
                        strcat(tmp, "|!|");
                        free(raw_info);
                    }
                }
            }
            /* Chiudo la concatenazione */
            strcat(tmp, "!>");

            /* Operazioni necessarie al passaggio dell'info di override */
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

            /* Mando il messaggio relativo alle informazioni che mi sono richieste
             * Sulla message queue relativa al pid */
            message.mesg_type = 1;
            sprintf(message.mesg_text, "%s", tmp);
            msgsnd(msgid_pid, &message, sizeof(message), 0);
        }
        if (flag_usr2) {
            flag_usr2 = 0;
            /* La finestra apre la pipe in lettura e ottiene cosa deve fare. */
            /* 0|.. -> spegni/accendi tutto */
            /* 1|.. -> attacca contenuto */

            /* Ricevo il messaggio relativo all'operazione da eseguire */
            msgrcv(msgid_pid, &message, sizeof(message), 1, 0);
            sprintf(tmp, "%s", message.mesg_text);

            /* Primo parametro è il codice dell'operazione */
            code = tmp[0] - '0';

            /* inizializzo la maschera booleana che indica se lo stato dei figli è concorde o meno con quello dell'hub */
            int j = 0;
            for (j = 0; j < MAX_CHILDREN; j++) {
                over_index[j] = -1;
            }

            /* Controllo lo stato */
            override = check_override(over_index);

            /* code 0 => cambiamento di status */
            if (code == 0) {
                /* cambio lo status */
                status = !status;
                for (i = 0; i < MAX_CHILDREN; i++) {
                    if (children_pids[i] != -1) {
                        /* Prendo le raw_info del figlio */
                        char* raw_info = get_raw_device_info(children_pids[i]);
                        /* le splitto */
                        char** split_info = split(raw_info);
                        /* Metodo che manda lo switch anche hai figli */
                        switch_child(atoi(split_info[2]), atoi(split_info[0]));
                    }
                }
            }
            /* Aggiunta di un nuovo figlio */
            if (code == 1) {
                /* Devo rimuovere i primi due caratteri per passare i parametri nel modo corretto */
                char* shifted_tmp = malloc(MAX_BUF_SIZE * sizeof(shifted_tmp));
                strcpy(shifted_tmp, tmp);
                shifted_tmp = shifted_tmp + 2;
                /* Splitto il tmp attuale */
                vars = split(shifted_tmp);
                /* Metodo che si occupa dell'aggiunta del figlio */
                __add_ex(vars, children_pids, MAX_CHILDREN);
                sleep(1); /* Blocco momentaneo al fine di garantire la sincronizzazione dei figli*/
                /* chiamata alla switch degli interruttori dei figli */
                switch_child(atoi(vars[2]), atoi(vars[0]));
                free(vars);
                free(shifted_tmp - 2);
            }
        }
        /* Chiamato nella link */
        if (flag_term) {
            term();
        }
        /* Chiamato nella del diretta del figlio */
        if (flag_urg) {
            flag_urg = 0;
            /* Riceve messaggio nella coda relativa al pid */
            msgrcv(msgid_pid, &message, sizeof(message), 1, 0);
            sprintf(tmp, "%s", message.mesg_text);
            /* splitto le variabili vars[0] = 2 vars[1] = pid del figlio eliminato. */
            vars = split(tmp);
            /*Cerco il figlio e setto il suo pid (nel vettore) a -1*/
            for (i = 0; i < MAX_CHILDREN; i++) {
                if (children_pids[i] == atoi(vars[1])) {
                    children_pids[i] = -1;
                }
            }
        }
        /* Chiamato nella del */
        if (flag_int) {
            int ppid = (int)getppid();
            /* Se fosse la shell la situazione sarebbe risolta nel metodo stesso */
            if (ppid != shellpid) {
                /* apre la messagequeue del padre (relativa a pid) */
                key_t key_ppid = ftok("/tmp/ipc/mqueues", ppid);
                int msgid_ppid = msgget(key_ppid, 0666 | IPC_CREAT);
                /* Scrive 2|pid nella message queue*/
                sprintf(message.mesg_text, "2|%d", pid);
                message.mesg_type = 1;
                msgsnd(msgid_ppid, &message, sizeof(message), 0);
                /*segnale al padre di aver scritto sulla coda */
                kill(ppid, SIGURG);
            }

            /* Itera sui figli se sono diversi da uno segnala la terminazione */
            int i = 0;
            for (i = 0; i < MAX_CHILDREN; i++) {
                if (children_pids[i] != -1) {
                    kill(children_pids[i], SIGTERM);
                }
            }

            /* Elimina la messagequeue relativa all'indice */
            msgctl(msgid_pid, IPC_RMID, NULL);
            exit(0);
        }
    }

    return 0;
}


/* a SIGTERM(=> O durante una link, oppure a seguito della del sul padre) ricevuta
 * un Hub scrive nella code relativa all'indice le informazioni relative ad i figli separati da "-" */
void term() {
    int i;
    char tmp[MAX_BUF_SIZE - sizeof(int)]; /* POI VA CONCATENATO */

    int count = 0;
    char intern[MAX_BUF_SIZE];
    char* info;

    /* inizia la concantenazione delle informazioni relative ai figli */
    sprintf(tmp, "-");
    for (i = 0; i < MAX_CHILDREN; i++) {
        if (children_pids[i] != -1) {
            count++;
            /* Richiede le info al proprio figlio */
            info = get_raw_device_info(children_pids[i]);
            /* Operazioni di concatenazione */
            sprintf(intern, "-%s", info);
            strcat(tmp, intern);
            /* Segnala la terminazione del figlio (Con SIGTERM dato che siamo in una link o in una del)*/
            kill(children_pids[i], SIGTERM);
        }
    }
    /* setto il tipo di messaggio */
    message.mesg_type = 1;

    /* Mando il messaggio nella coda relativa all'indice */
    sprintf(message.mesg_text, "%d%s", count, tmp);
    msgsnd(msgid, &message, sizeof(message), 0);

    /* distruggo la message queue relativa al pid */
    msgctl(msgid_pid, IPC_RMID, NULL);
    exit(0);
}


/* Funzione chiamata in apertura che si occupa della lettura della coda relativa all'indice
 * così da ottenere i figli che vanno aggiunti al proprio vettore dei figli */
void read_msgqueue(int msgid) {
    int n_devices;
    int ret;
    int q, j;
    char n_dev_str[100];
    int __count;
    char tmp_buf[MAX_BUF_SIZE];
    char** vars;
    char** son_j;

    /* Lettura messagequeu relativa indice se ritorna -1 è vuota, oppure già stata letta */
    ret = msgrcv(msgid, &message, sizeof(message), 1, IPC_NOWAIT);
    if (ret != -1) {
        /* Ottengo il numero di figli presenti*/
        q = 0;
        while (!(message.mesg_text[q] == '-')) {
            n_dev_str[q] = message.mesg_text[q];
            q++;
        }
        n_dev_str[q] = '\0';
        n_devices = atoi(n_dev_str);
        /* Se maggiore di 0 => Li aggiungo tramite add_ex */
        if (n_devices > 0) {
            __count = n_devices;
            sprintf(tmp_buf, "%s", message.mesg_text);
            vars = NULL;
            /* vars conterrà le raw info dei figli in ogni indice abbiamo una stringa di raw_info */
            vars = split_sons(tmp_buf, __count);
            j = 0;
            while (j <= __count) {
                if (j >= 1) {
                    /* splitto le raw_info in var[j] */
                    son_j = split(vars[j]);
                    /* aggiungo il figlio */
                    __add_ex(son_j, children_pids, MAX_CHILDREN);
                }
                j++;
            }
        }
    }
}
