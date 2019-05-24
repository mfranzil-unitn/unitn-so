#include "../actions.h"
#include "../util.h"

/* HUB = 4 */

int shellpid;

int fd;           /* file descriptor della pipe verso il padre */
int pid, __index; /* variabili di stato */
int status = 0;   /* interruttore accensione */

int children_pids[MAX_CHILDREN];
int override = 0;
char info[MAX_BUF_SIZE];

key_t key;
int msgid;
key_t key_pid;
int msgid_pid;

volatile int flag_usr1 = 0;
volatile int flag_usr2 = 0;
volatile int flag_term = 0;

void term();
void read_msgqueue(int msgid, int* device_pids);
void __switch_hub(int pid, char* action, char* position, int* children_pids);

void sighandler_int(int sig) {
    if (sig == SIGUSR1) {
        printf("SIGUSR1 SONO HUB %d CON %d\n", __index, pid);
        flag_usr1 = 1;
    }
    if (sig == SIGUSR2) {
        flag_usr2 = 1;
    }
    if (sig == SIGTERM) {
        flag_term = 1;
    }
}

/*Itera sui figli, in realtà fino a MAX_CHILDREN, e controlla che gli stati siano congruenti. */
/*E modifica il vettore over_index Maschera di bit. */
int check_override(int* over_index) {
    int i = 0;
    int ret = 0;
    char** vars;

    for (i = 0; i < MAX_CHILDREN; i++) {
        if (children_pids[i] != -1) {
            char* raw_info = get_raw_device_info(children_pids[i]);

            if (raw_info == NULL) {
                continue;
            }

            vars = split(raw_info);
            if (atoi(vars[3]) != status) {
                over_index[i] = 1;
                ret = 1;
            }
        }
    }

    free(vars);

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

    this_pipe = argv[2];
    pid = getpid();
    __index = atoi(argv[1]);
    fd = open(this_pipe, O_RDWR);

    for (i = 0; i < MAX_CHILDREN; i++) {
        children_pids[i] = -1;
    }

    key = ftok("/tmp/ipc/mqueues", __index);
    msgid = msgget(key, 0666 | IPC_CREAT);

    read_msgqueue(msgid, children_pids);
    /*printf("HI BRO I'M HUB %d\n", __index); */
    for (i = 0; i < MAX_CHILDREN; i++) {
        if (children_pids[i] != -1) {
            /*printf("Dispositivo %d: %d\n", i, children_pids[i]); */
        }
    }
    shellpid = get_shell_pid();

    key_pid = ftok("/tmp/ipc/mqueues", pid);
    msgid_pid = msgget(key_pid, 0666 | IPC_CREAT);
    /*printf(" KEY %d MSGID: %d\n",key_pid, msgid_pid); */

    signal(SIGCHLD, SIG_IGN);
    signal(SIGTERM, sighandler_int);
    signal(SIGUSR1, sighandler_int);
    signal(SIGUSR2, sighandler_int);

    while (1) {
        __index = atoi(argv[1]);

        if (flag_usr1) {
            flag_usr1 = 0;
            override = 0;
            /*printf("hub usr1: %d\n", pid); */
            /* bisogna controllare se i dispositivi sono allineati o meno (override) */

            /* conto i dispositivi connessi */
            connected = 0;

            for (i = 0; i < MAX_CHILDREN; i++) {
                if (children_pids[i] != -1) {
                    /*printf("Children pids[%d]: %d\n", i, children_pids[i] ); */
                    connected++;
                }
            }

            /*
            sprintf(tmp, "4|%d|%d|%d|%d|<!|",
                    pid, __index, status, connected);
            */

            /* Stampo nel buffer tante volte quanti device ho */
            /*
            for (i = 0; i < MAX_CHILDREN; i++) {
                if (children_pids[i] != -1) {
                    raw_info = get_raw_device_info(children_pids[i]);
                    if(raw_info != NULL){
                    //printf("INFO PER FIGLIO: %d di HUB %d: %s\n", children_pids[i], pid, raw_info);
                    strcat(tmp, raw_info);
                    strcat(tmp, "|!|");
                    free(raw_info);
                    }else{
                        printf("Ti ho beccato, pezzo di merda\n");
                    }
                }
            }

            strcat(tmp, "!>");
            //printf("TMP DI HUB %d: %s\n",pid,  tmp);
            sprintf(message.mesg_text, "%s", tmp);
            printf("HUB message: %s\n", message.mesg_text);
            msgsnd(msgid_pid, &message, sizeof(message), 0);
            */

            /*write(fd, tmp, MAX_BUF_SIZE); */
            /*printf("INFO SENT\n"); */

            sprintf(tmp, "4|%d|%d|%d|%d|<!|",
                    pid, __index, status, connected);

            /* Stampo nel buffer tante volte quanti device ho */

            for (i = 0; i < MAX_CHILDREN; i++) {
                if (children_pids[i] != -1) {
                    raw_info = get_raw_device_info(children_pids[i]);
                    /*if(atoi(raw_info[3]) != status){
                        override = 1;
                    }*/
                    if (raw_info != NULL) {
                        /*printf("INFO PER FIGLIO: %d di HUB %d: %s\n", children_pids[i], pid, raw_info); */
                        strcat(tmp, raw_info);
                        strcat(tmp, "|!|");
                        free(raw_info);
                    } else {
                        printf("Ti ho beccato, pezzo di merda\n");
                    }
                }
            }

            strcat(tmp, "!>");
            /*printf("TMP DI HUB %d: %s\n",pid,  tmp); */
            message.mesg_type = 1;
            sprintf(message.mesg_text, "%s", tmp);
            /*printf("HUB message: %s\n", message.mesg_text); */
            msgsnd(msgid_pid, &message, sizeof(message), 0);
            /*printf("MESSAGE SENT %s\n", message.mesg_text); */
            if (override) {
                printf("OVERRIDE\n");
            }
        }
        if (flag_usr2) {
            flag_usr2 = 0;
            /* La finestra apre la pipe in lettura e ottiene cosa deve fare. */
            /* 0|.. -> spegni/accendi tutto */
            /* 1|.. -> attacca contenuto */
            /* 2|.. -> toglie contenuto */

            printf("hub usr2: %d\n", pid);
            /*read(fd, mall_tmp, MAX_BUF_SIZE);
            printf("End Read: %s\n\n", mall_tmp);*/
            msgrcv(msgid_pid, &message, sizeof(message), 1, 0);
            sprintf(tmp, "%s", message.mesg_text);
            printf("End Read: %s\n\n", tmp);
            code = tmp[0] - '0';
            /*printf("hub code: %d\n", code); */

            //check_override(over_index);
            if (code == 0) {
                /*printf("CODE 0\n"); */
                status = !status;
                for (i = 0; i < MAX_CHILDREN; i++) {
                    if (children_pids[i] != -1 && !over_index[i]) {
                        printf("Switching children[%d]; %d\n", i, children_pids[i]);
                        __switch_hub(children_pids[i], "accensione", status ? "on" : "off", children_pids);
                    }
                }
            }
            if (code == 1) {
                /* Devo rimuovere i primi due caratteri per passare i parametri nel modo corretto */
                char* shifted_tmp = malloc(MAX_BUF_SIZE * sizeof(shifted_tmp));
                strcpy(shifted_tmp, tmp);
                shifted_tmp = shifted_tmp + 2;
                vars = split(shifted_tmp);
                __add_ex(vars, children_pids);
                free(vars);
                free(shifted_tmp - 2);
            }
            if (code == 2) {
                /*printf("CODE 2\n"); */
                vars = split(tmp);
                for (i = 0; i < MAX_CHILDREN; i++) {
                    if (children_pids[i] == atoi(vars[1])) {
                        /*printf("BECCATO: childern_Pids: %d, atoi: %d\n", children_pids[i], atoi(vars[1])); */
                        children_pids[i] = -1;
                    }
                }
            }
        }
        if (flag_term) {
            term();
        }
        sleep(10);
    }

    return 0;
}

void term() {
    int done = 1;
    int i;
    char tmp[MAX_BUF_SIZE - sizeof(int)]; /* POI VA CONCATENATO */

    int count = 0;
    char intern[MAX_BUF_SIZE];
    char* info;

    /*printf("IN TERM FOR HUB: %d\n", __index); */
    /*  if (ppid != shellpid) {
        kill(ppid, SIGUSR2);
        get_pipe_name(ppid, pipe_str); 
        fd = open(pipe_str, O_RDWR);
        sprintf(tmp, "2|%d", (int)getpid());
        write(fd, tmp, sizeof(tmp));
    }

    for (i = 0; i < MAX_CHILDREN; i++) {
        if (children_pids[i] != -1) {
            printf("Chiamata link_ex per figlio %d\n", children_pids[i]);
            ret = __link_ex(children_pids[i], ppid, shellpid);
            if (ret != 1) {
                done = 0;
            }
        }
    }
*/

    sprintf(tmp, "-");
    for (i = 0; i < MAX_CHILDREN; i++) {
        if (children_pids[i] != -1) {
            /*printf("Trying to send pids\n"); */
            count++;
            /*printf("Trying to get INFO\n"); */
            info = get_raw_device_info(children_pids[i]);
            /*printf("INFO WE HAVE!: %s\n", info); */
            sprintf(intern, "-%s", info);
            /*printf("INTERN: %s\n", intern); */
            strcat(tmp, intern);
            kill(children_pids[i], SIGTERM);
        }
    }
    message.mesg_type = 1;

    sprintf(message.mesg_text, "%d%s", count, tmp);
    msgsnd(msgid, &message, sizeof(message), 0);

    /*int ret = __link_ex(children_pids, ppid, shellpid); */

    if (done) {
        exit(0);
    } else {
        printf("Errore nell'eliminazione\n");
    }
}

void read_msgqueue(int msgid, int* device_pids) {
    int n_devices;
    int ret;
    int q, j;
    char n_dev_str[100];
    int __count;
    char tmp_buf[MAX_BUF_SIZE];
    char** vars;
    char** son_j;

    printf("Lettura figlio da aggiungere...\n");
    ret = msgrcv(msgid, &message, sizeof(message), 1, IPC_NOWAIT);
    printf("Dovrei aggiungere figli: %s\n", message.mesg_text);
    if (ret != -1) {
        q = 0;
        while (!(message.mesg_text[q] == '-')) {
            n_dev_str[q] = message.mesg_text[q];
            q++;
        }
        n_dev_str[q] = '\0';
        n_devices = atoi(n_dev_str);
        if (n_devices > 0) {
            __count = n_devices;
            sprintf(tmp_buf, "%s", message.mesg_text);
            vars = NULL;
            vars = split_sons(tmp_buf, __count);
            j = 0;
            while (j <= __count) {
                if (j >= 1) {
                    printf("\nVars %d: %s\n", j, vars[j]);
                    son_j = split(vars[j]);
                    __add_ex(son_j, children_pids);
                    printf("\nADD_EX GOOD\n");
                }
                j++;
            }
        }
    }
}

void __switch_hub(int pid, char* action, char* position, int* children_pids) {
    /* Prova a impostare un interruttore ACTION su POSITION di un certo DEVICE */
    char* device_info;
    int fd, status;
    char** vars;
    char pipe_str[MAX_BUF_SIZE];
    char pipe_message[MAX_BUF_SIZE]; /* buffer per la pipe */

    device_info = get_raw_device_info(pid);

    if (pid == -1) {
        printf("Errore! Non esiste questo dispositivo.\n");
        return;
    }

    if (device_info == NULL) {
        printf("Errore di connessione (PID %d)\n", pid);
        return;
    }

    vars = split(device_info);
    /*get_pipe_name(pid, pipe_str);

    fd = open(pipe_str, O_RDWR);*/

    key_t key = ftok("/tmp/ipc/mqueues", pid);
    int msgid = msgget(key, 0666 | IPC_CREAT);
    message.mesg_type = 1;

    if (strcmp(vars[0], BULB_S) == 0) { /* Lampadina */
        if (strcmp(action, "accensione") == 0) {
            status = atoi(vars[3]);
            sprintf(pipe_message, "0|0");
            /*printf("pipe message: %s\n", pipe_message); */

            if (strcmp(position, "on") == 0 && status == 0) {
                sprintf(message.mesg_text, "%s", pipe_message);
                msgsnd(msgid, &message, sizeof(message), 0);
                kill(pid, SIGUSR2);
                printf("Lampadina accesa.\n");
            } else if (strcmp(position, "off") == 0 && status == 1) {
                sprintf(message.mesg_text, "%s", pipe_message);
                msgsnd(msgid, &message, sizeof(message), 0);
                kill(pid, SIGUSR2); /* sleep(1) */
                printf("Lampadina spenta.\n");
            } else if (strcmp(position, "off") == 0 && status == 0) { /* Spengo una lampadina spenta */
                printf("Stai provando a spegnere una lampadina spenta!\n");
            } else if (strcmp(position, "on") == 0 && status == 1) { /* Accendo una lampadina accesa */
                printf("Stai provando a accendere una lampadina accesa!\n");
            } else {
                printf("Sintassi non corretta. Sintassi: switch <bulb> accensione <on/off>\n");
            }
        } else {
            printf("Operazione non permessa su una lampadina!\nOperazioni permesse: accensione\n");
        }
    } else if (strcmp(vars[0], FRIDGE_S) == 0) { /* Fridge */
        if (strcmp(action, "apertura") == 0) {
            status = atoi(vars[3]);
            sprintf(pipe_message, "0|0");

            if (strcmp(position, "on") == 0 && status == 0) {
                sprintf(message.mesg_text, "%s", pipe_message);
                msgsnd(msgid, &message, sizeof(message), 0);
                kill(pid, SIGUSR2); /* sleep(1) */
                printf("Frigorifero aperto.\n");
            } else if (strcmp(position, "off") == 0 && status == 1) {
                sprintf(message.mesg_text, "%s", pipe_message);
                msgsnd(msgid, &message, sizeof(message), 0);
                kill(pid, SIGUSR2); /* sleep(1) */
                printf("Frigorifero chiuso.\n");
            } else if (strcmp(position, "off") == 0 && status == 0) { /* Chiudo frigo già chiuso */
                printf("Stai provando a chiudere un frigorifero già chiuso.\n");
            } else if (strcmp(position, "on") == 0 && status == 1) { /* Apro frigo già aperto */
                printf("Stai provando a aprire un frigorifero già aperto.\n");
            } else {
                printf("Sintassi non corretta. Sintassi: switch <fridge> apertura <on/off>\n");
            }
        } else if (strcmp(action, "temperatura") == 0) {
            if ((atoi(position) >= -10 && atoi(position) < 0) || (atoi(position) > 0 && atoi(position) <= 15) || strcmp(position, "0") == 0) {
                sprintf(pipe_message, "1|%s", position);
                sprintf(message.mesg_text, "%s", pipe_message);
                msgsnd(msgid, &message, sizeof(message), 0);
                kill(pid, SIGUSR2); /* sleep(1) */
                printf("Temperatura modificata con successo a %s°C.\n", position);
            } else {
                printf("Sintassi non corretta. Sintassi: switch <fridge> temperatura <-10 - 15>\n");
            }
        } else if (strcmp(action, "delay") == 0) {
            if ((atoi(position) > 0 && atoi(position) <= (60 * 5)) || strcmp(position, "0") == 0) { /* Massimo 5 minuti */
                sprintf(pipe_message, "2|%s", position);
                sprintf(message.mesg_text, "%s", pipe_message);
                msgsnd(msgid, &message, sizeof(message), 0);
                kill(pid, SIGUSR2); /* sleep(1) */
                printf("Tempo di richiusura automatico modificato con successo a %s secondi.\n", position);
            } else {
                printf("Sintassi non corretta. Sintassi: switch <fridge> delay <0-300>.\n");
            }
        } else if (strcmp(action, "riempimento") == 0) { /* Possibile solo manualmente (launcher) */
            if ((atoi(position) > 0 && atoi(position) <= 100) || strcmp(position, "0") == 0) {
                sprintf(pipe_message, "3|%s", position);
                sprintf(message.mesg_text, "%s", pipe_message);
                msgsnd(msgid, &message, sizeof(message), 0);
                kill(pid, SIGUSR2); /* sleep(1) */
                printf("Percentuale di riempimento modificato con successo a %s. \n", position);
            } else {
                printf("Sintassi non corretta. Sintassi: switch <fridge> riempimento <0-100>.\n");
            }
        } else {
            printf("Operazione non permessa su un frigorifero! Operazioni permesse: <temperatura/apertura/delay/riempimento>\n");
        }

    } else if (strcmp(vars[0], WINDOW_S) == 0) { /* Window */
        if (strcmp(action, "apertura") == 0 || strcmp(action, "chiusura") == 0) {
            if (((strcmp(action, "apertura") != 0) || (strcmp(action, "apertura") == 0 && strcmp(position, "off") == 0)) &&
                ((strcmp(action, "chiusura") != 0) || (strcmp(action, "chiusura") == 0 && strcmp(position, "off") == 0))) {
                printf("Operazione non permessa: i pulsanti sono solo attivi!\n");
                /* se off non permetto */
                return;
            }

            status = atoi(vars[3]);
            sprintf(pipe_message, "0|0");

            if (strcmp(action, "apertura") == 0 && status == 0) {
                sprintf(message.mesg_text, "%s", pipe_message);
                msgsnd(msgid, &message, sizeof(message), 0);
                kill(pid, SIGUSR2); /* sleep(1) */
                printf("Finestra aperta.\n");
            } else if (strcmp(action, "chiusura") == 0 && status == 1) {
                sprintf(message.mesg_text, "%s", pipe_message);
                msgsnd(msgid, &message, sizeof(message), 0);
                kill(pid, SIGUSR2); /* sleep(1) */
                printf("Finestra chiusa.\n");
            } else {
                printf("Operazione non permessa: pulsante già premuto.\n");
            }
        } else {
            printf("Operazione non permessa su una finestra! Operazioni permesse: <apertura/chiusura>.");
        }
    } else if (strcmp(vars[0], HUB_S) == 0) { /* Hub */
        if (strcmp(action, "accensione") == 0) {
            status = atoi(vars[3]);
            sprintf(pipe_message, "0|0");

            if (strcmp(position, "on") == 0 && status == 0) {
                sprintf(message.mesg_text, "%s", pipe_message);
                msgsnd(msgid, &message, sizeof(message), 0);
                kill(pid, SIGUSR2); /* sleep(1) */
                printf("Hub acceso.\n");
            } else if (strcmp(position, "off") == 0 && status == 1) {
                sprintf(message.mesg_text, "%s", pipe_message);
                msgsnd(msgid, &message, sizeof(message), 0);
                kill(pid, SIGUSR2); /* sleep(1) */
                printf("Hub spento.\n");
            } else if (strcmp(position, "off") == 0 && status == 0) { /* Spengo un hub spento */
                printf("Stai provando a spegnere un hub spenta!\n");
            } else if (strcmp(position, "on") == 0 && status == 1) { /* Accendo un hub acceso */
                printf("Stai provando a accendere un hub acceso!\n");
            } else {
                printf("Sintassi non corretta. Sintassi: switch <hub> accensione <on/off>\n");
            }
        } else {
            printf("Operazione non permessa su un hub!\nOperazioni permesse: accensione\n");
        }
    } else if (strcmp(vars[0], TIMER_S) == 0) {
        if (strcmp(action, "orario") == 0) {
            int h_start, m_start, h_end, m_end;
            /* Aggiungere controlli sugli orari */
            int scan = sscanf(position, "%d:%d->%d:%d", &h_start, &m_start, &h_end, &m_end);
            if (scan != 4 || h_start < 0 || h_start > 23 || h_end < 0 || h_end > 59 || h_start > h_end || m_start > m_end) {
                printf("Formattazione degli orari sbagliata. Formato (24 ore): \"HH:MM -> HH:MM\"\n");
            } else {
                sprintf(pipe_message, "0|%d|%d|%d|%d", h_start, m_start, h_end, m_end);
                sprintf(message.mesg_text, "%s", pipe_message);
                msgsnd(msgid, &message, sizeof(message), 0);
                kill(pid, SIGUSR2);
                printf("Timer settato dalle ore %d:%d alle ore %d:%d\n", h_start, m_start, h_end, m_end);
            }
        } else {
            printf("Operazione non permessa su un hub!\nOperazioni permesse: orario\n");
        }
    } else { /* tutti gli altri dispositivi */
        printf("Dispositivo non supportato.\n");
    }
    close(fd);
    free(vars);
}