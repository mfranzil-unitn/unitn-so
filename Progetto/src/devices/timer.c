#include "../actions.h"
#include "../util.h"

/* TIMER = 5 */

int shellpid;

/* Registri del timer */
int fd;           /* file descriptor della pipe verso il padre */
int pid, __index; /* variabili di stato */
int status = 0;   /* interruttore accensione */

/* Registri per il figlio - array usato per intercompatibilità */
int children_pids[1];
int children_index = -1; /* usato in cache*/
int device_type = -1;

key_t key;
int msgid;
key_t key_pid;
int msgid_pid;

struct tm tm_start;
struct tm tm_end;
struct tm tm_current;

volatile int flag_usr1 = 0;
volatile int flag_usr2 = 0;
volatile int flag_term = 0;
volatile int flag_alarm = 0;
volatile int flag_cont = 0;

void switch_child() {
    char switch_names[5][MAX_BUF_SIZE];

    if (children_pids[0] == -1) {
        return;
    }
    sprintf(switch_names[0], "-");          /* Per shiftare gli indici così che corrispondano */
    sprintf(switch_names[1], "accensione"); /* Bulb */
    sprintf(switch_names[2], "apertura");   /* Fridge */
    sprintf(switch_names[3], "apertura");   /* Window */
    sprintf(switch_names[4], "accensione"); /* Hub */

    __switch(children_index, switch_names[device_type], status ? "on" : "off", children_pids);
}

void check_time() {
    tm_current = *localtime(&(time_t){time(NULL)}); /*
    if (tm_current.tm_hour >= tm_start.tm_hour && tm_current.tm_min >= tm_start.tm_min) {
        status = 1;
        alarm(difftime(mktime(&tm_end), mktime(&tm_current)));
        switch_child();
    } else if (tm_current.tm_hour >= tm_end.tm_hour && tm_current.tm_min >= tm_end.tm_min) {
        status = 0;
        alarm(difftime(mktime(&tm_start), mktime(&tm_current)));
        switch_child();
    }*/
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
    if (sig == SIGCONT) {
        flag_cont = 1;
    }
}

int main(int argc, char* argv[]) {
    /* argv = [./timer, indice, /tmp/indice]; */
    char tmp[MAX_BUF_SIZE]; /* Buffer per le pipe*/
    /*har ppid_pipe[MAX_BUF_SIZE]; Pipe per il padre*/
    char* this_pipe = NULL; /* Pipe di questo dispositivo */

    char** vars = NULL;
    int mode;

    char* raw_info;
    char* shifted_tmp;

    int ret;

    this_pipe = argv[2];
    pid = getpid();
    __index = atoi(argv[1]);
    fd = open(this_pipe, O_RDWR);

    shellpid = get_shell_pid();

    tm_start = *localtime(&(time_t){time(NULL)});
    tm_end = *localtime(&(time_t){time(NULL)});

    tm_start.tm_hour = 8;
    tm_start.tm_min = 0;
    tm_end.tm_hour = 9;
    tm_end.tm_min = 0;

    flag_alarm = 1;
    children_pids[0] = -1;

    signal(SIGTERM, sighandler_int);
    signal(SIGUSR1, sighandler_int);
    signal(SIGUSR2, sighandler_int);
    signal(SIGALRM, sighandler_int);

    key = ftok("/tmp/ipc/mqueues", pid);
    msgid = msgget(key, 0666 | IPC_CREAT);

    while (1) {
        if (flag_usr1) {
            flag_usr1 = 0;
            sprintf(tmp, "5|%d|%d|%d|%d|%d|%d|%d|%d|<!|",
                    pid, __index, status,
                    tm_start.tm_hour, tm_start.tm_min,
                    tm_end.tm_hour, tm_end.tm_min,
                    children_pids[0] != -1);
            if (children_pids[0] != -1) {
                raw_info = get_raw_device_info(children_pids[0]);
                if (raw_info != NULL) {
                    strcat(tmp, raw_info);
                    strcat(tmp, "|!|");
                    free(raw_info);
                }
            }
            strcat(tmp, "!>");
            message.mesg_type = 1;
            sprintf(message.mesg_text, "%s", tmp);
            msgsnd(msgid, &message, sizeof(message), 0);
        }

        if (flag_usr2) {
            flag_usr2 = 0;
            /* 
                Al ricevimento del segnale, la finestra apre la pipe in lettura e ottiene cosa deve fare.
                0|ORA|MINUTI|ORAFINE|MINUTIFINE -> imposta timer
                1|FIGLIO -> Aggiungi figlio
                2|PID -> Rimuovi figlio
            */
            read(fd, tmp, MAX_BUF_SIZE);
            mode = tmp[0] - '0';

            /* lprintf("Entering user2\n");*/
            if (mode == 0) {
                vars = split_fixed(tmp, 5);

                tm_start = *localtime(&(time_t){time(NULL)});
                tm_end = *localtime(&(time_t){time(NULL)});

                tm_start.tm_hour = atoi(vars[1]);
                tm_start.tm_min = atoi(vars[2]);
                tm_end.tm_hour = atoi(vars[3]);
                tm_end.tm_min = atoi(vars[4]);

                flag_alarm = 1;
            } else if (mode == 1) {
                shifted_tmp = malloc(MAX_BUF_SIZE * sizeof(shifted_tmp));
                strcpy(shifted_tmp, tmp);
                shifted_tmp = shifted_tmp + 2;
                vars = split(shifted_tmp);
                __add_ex(vars, children_pids);
                device_type = atoi(vars[0]);
                children_index = atoi(vars[2]);
                free(vars);
                free(shifted_tmp - 2);
            } else if (mode == 2) {
                vars = split(tmp);
                if (children_pids[0] == atoi(vars[1])) {
                    children_pids[0] = -1;
                    children_index = -1;
                    device_type = -1;
                }
            }
        }

        if (flag_term) {
            term();
        }

        if (flag_cont) {
            flag_cont = 0;
            ret = msgrcv(msgid_pid, &message, sizeof(message), 1, IPC_NOWAIT);
            if (ret != -1) {
                if (children_pids[0] == atoi(message.mesg_text)) {
                    children_pids[0] = -1;
                }
            }
        }

        if (flag_alarm) {
            check_time();
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

    sprintf(tmp, "-");
    if (children_pids[0] != -1) {
        count++;
        info = get_raw_device_info(children_pids[0]);
        sprintf(intern, "-%s", info);
        strcat(tmp, intern);
        kill(children_pids[0], SIGTERM);
    }
    message.mesg_type = 1;

    sprintf(message.mesg_text, "%d%s", count, tmp);
    msgsnd(msgid, &message, sizeof(message), 0);

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

    ret = msgrcv(msgid, &message, sizeof(message), 1, IPC_NOWAIT);
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
