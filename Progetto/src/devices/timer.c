#include "../actions.h"
#include "../util.h"

/* TIMER = 5 */

struct rctm {
    int hour;
    int min;
};

int shellpid;

/* Registri del timer */
int fd;           /* file descriptor della pipe verso il padre */
int pid, __index; /* variabili di stato */
int status = 0;   /* interruttore accensione */

/* Registri per il figlio - array usato per intercompatibilità */
int children_pids[1];
int override = 0;
char info[MAX_BUF_SIZE];

int children_index = -1; /* usato in cache */
int device_type = -1;

key_t key;
int msgid;

key_t key_pid;
int msgid_pid;

struct rctm tm_start;
struct rctm tm_end;

volatile int flag_usr1 = 0;
volatile int flag_usr2 = 0;
volatile int flag_term = 0;
volatile int flag_alarm = 0;
volatile int flag_int = 0;

void term();
void read_msgqueue(int msgid);

/* Restituisce 1 solo se i figli sono in override */
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

    //free(vars);

    return ret;
}

void switch_child() {
    char switch_names[6][MAX_BUF_SIZE] = {"-", "accensione", "apertura", "apertura", "accensione", "accensione"};

    if (children_pids[0] == -1) {
        return;
    }

    __switch_index(children_index, switch_names[device_type], status ? "on" : "off", children_pids);
}

void check_time() {
    struct tm tmp = *localtime(&(time_t){time(NULL)});
    struct rctm tm_current;
    const int SECONDS_IN_A_DAY = 86400;

    tm_current.hour = tmp.tm_hour;
    tm_current.min = tmp.tm_min;

    int tm_current_seconds = 60 * 60 * tm_current.hour + 60 * tm_current.min + tmp.tm_sec;
    int tm_end_seconds = 60 * 60 * tm_end.hour + 60 * tm_end.min;
    int tm_start_seconds = 60 * 60 * tm_start.hour + 60 * tm_start.min;

    //printf("Current: %d, Start %d, End %d\n", tm_current_seconds, tm_start_seconds, tm_end_seconds);
    if (tm_current_seconds >= tm_start_seconds && tm_current_seconds < tm_end_seconds) {
        //printf("Fascia on\n");
        /* Sono nella "fascia oraria" */
        status = 1;
        alarm((tm_end_seconds - tm_current_seconds) % SECONDS_IN_A_DAY);
        switch_child();
    } else {
        //printf("Fascia off\n");
        status = 0;
        alarm((tm_start_seconds - tm_current_seconds) % SECONDS_IN_A_DAY);
        switch_child();
    }
    flag_alarm = 0;
    return;
}

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

    this_pipe = argv[2];
    pid = getpid();
    __index = atoi(argv[1]);
    fd = open(this_pipe, O_RDWR);

    // -------------------------------------
    shellpid = get_shell_pid();
    // -------------------------------------
    printf("SHPID %d\n", shellpid);
    getchar();

    tm_start.hour = 8;
    tm_start.min = 0;
    tm_end.hour = 9;
    tm_end.min = 0;

    flag_alarm = 1;
    children_pids[0] = -1;

    signal(SIGCHLD, SIG_IGN);
    signal(SIGTERM, sighandler_int);
    signal(SIGUSR1, sighandler_int);
    signal(SIGUSR2, sighandler_int);
    signal(SIGALRM, sighandler_int);
    signal(SIGINT, sighandler_int);

    key = ftok("/tmp/ipc/mqueues", __index);
    msgid = msgget(key, 0666 | IPC_CREAT);

    read_msgqueue(msgid);

    key_pid = ftok("/tmp/ipc/mqueues", pid);
    msgid_pid = msgget(key_pid, 0666 | IPC_CREAT);

    while (1) {
        __index = atoi(argv[1]);
        if (flag_usr1) {
            flag_usr1 = 0;
            override = 0;

            // IDEA: SETTARE a 2 STATUS SE OVERRIDE

            sprintf(tmp, "5|%d|%d|%d|%d|%d|%d|%d|%d|<!|",
                    pid, __index, status,
                    tm_start.hour, tm_start.min,
                    tm_end.hour, tm_end.min,
                    children_pids[0] != -1);
            if (children_pids[0] != -1) {
                char* raw_info = get_raw_device_info(children_pids[0]);
                if (raw_info != NULL) {
                    strcat(tmp, raw_info);
                    strcat(tmp, "|!|");
                    free(raw_info);
                }
            }
            strcat(tmp, "!>");

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
            msgrcv(msgid_pid, &message, sizeof(message), 1, 0);
            sprintf(tmp, "%s", message.mesg_text);

            code = tmp[0] - '0';

            if (code == 0) {
                override = check_override(over_index);
                status = !status;
                if (children_pids[0] != -1 /*&& !over_index[0]*/) {
                    switch_child();
                    //char* raw_info = get_raw_device_info(children_pids[0]);
                    //__switch(children_pids[0], "accensione", status ? "on" : "off", raw_info);
                }
            }
            if (code == 1) {
                /* Devo rimuovere i primi due caratteri per passare i parametri nel modo corretto */
                char* shifted_tmp = malloc(MAX_BUF_SIZE * sizeof(shifted_tmp));
                strcpy(shifted_tmp, tmp);
                shifted_tmp = shifted_tmp + 2;

                vars = split(shifted_tmp);
                __add_ex(vars, children_pids, 1);

                device_type = atoi(vars[0]);
                children_index = atoi(vars[2]);

                sleep(2);
                switch_child();
                //__switch_index(children_index, "accensione", status ? "on" : "off", children_pids);
                free(vars);
                free(shifted_tmp - 2);
            }
            if (code == 2) {
                vars = split(tmp);
                if (children_pids[0] == atoi(vars[1])) {
                    children_pids[0] = -1;
                    children_index = -1;
                    device_type = -1;
                }
            }
            if (code == 3) {
                vars = split_fixed(tmp, 5);

                tm_start.hour = atoi(vars[1]);
                tm_start.min = atoi(vars[2]);
                tm_end.hour = atoi(vars[3]);
                tm_end.min = atoi(vars[4]);

                flag_alarm = 1;
            }
        }
        if (flag_term) {
            term();
        }

        if (flag_alarm) {
            check_time();
        }
        if (flag_int) {
            int ppid = (int)getppid();
            if (ppid != shellpid) {
                key_t key_ppid = ftok("/tmp/ipc/mqueues", ppid);
                int msgid_ppid = msgget(key_ppid, 0666 | IPC_CREAT);
                sprintf(message.mesg_text, "2|%d", pid);
                message.mesg_type = 1;
                msgsnd(msgid_ppid, &message, sizeof(message), 0);
                kill(ppid, SIGURG);
            }
            char* info;
            char* intern;
            if (children_pids[0] != -1) {
                info = get_raw_device_info(children_pids[0]);
                /*printf("INFO WE HAVE!: %s\n", info); */
                sprintf(intern, "-%s", info);
                /*printf("INTERN: %s\n", intern); */
                strcat(tmp, intern);
                kill(children_pids[0], SIGTERM);
            }

            msgctl(msgid_pid, IPC_RMID, NULL);
            exit(0);
        }
        //sleep(10);
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

    sprintf(tmp, "-");

    //printf("PID: %d", children_pids[0]);

    if (children_pids[0] != -1) {
        char* son_info = get_raw_device_info(children_pids[0]);
        char intern[MAX_BUF_SIZE];
        sprintf(intern, "-%s", son_info);
        strcat(tmp, son_info);

        sprintf(message.mesg_text, "1%s", tmp);
    } else {
        sprintf(message.mesg_text, "0%s", tmp);
    }
    message.mesg_type = 1;
    msgsnd(msgid, &message, sizeof(message), 0);

    /*int ret = __link_ex(children_pids, ppid, shellpid); */

    if (done) {
        msgctl(msgid_pid, IPC_RMID, NULL);
        exit(0);
    } else {
        printf("Errore nell'eliminazione.\n");
    }
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

    printf("Lettura figlio da aggiungere...\n");
    ret = msgrcv(msgid, &message, sizeof(message), 1, IPC_NOWAIT);
    printf("Dovrei aggiungere figli: %s\n", message.mesg_text);

    //getchar();
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
            //j = 0;
            // while (j <= __count) {
            //if (j >= 1) {
            //   printf("\nVars %d: %s\n", j, vars[j]);
            son_j = split(vars[1]);
            __add_ex(son_j, children_pids, 1);
            //   printf("\nADD_EX GOOD\n");
        }
        //j++;
    }
}