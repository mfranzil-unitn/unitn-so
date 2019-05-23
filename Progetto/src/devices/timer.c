#include "../actions.h"
#include "../util.h"

/* TIMER = 5 */

int shellpid;

/* Registri del timer */
int fd;           /* file descriptor della pipe verso il padre */
int pid, __index; /* variabili di stato */
int status = 0;   /* interruttore accensione */

/* Registri per il figlio - array usato per intercompatibilità */
int children_pids[1] = {-1};
int children_index = -1; /* usato in cache*/
int device_type = -1;

struct tm tm_start;
struct tm tm_end;
struct tm tm_current;

volatile int flag_usr1 = 0;
volatile int flag_usr2 = 0;
volatile int flag_term = 0;

void switch_child() {
    if (children_pids[0] == -1) {
        return;
    }
    char* switch_names = {"-",           /* Per shiftare gli indici così che corrispondano */
                          "accensione",  /* Bulb */
                          "apertura",    /* Fridge */
                          "apertura",    /* Window */
                          "accensione"}; /* Hub */

    __switch(children_index, switch_names[device_type], status ? "on" : "off", children_pids);
}

void check_time() {
    tm_current = *localtime(&(time_t){time(NULL)});
    if (tm_current.tm_hour >= tm_start.tm_hour && tm_current.tm_min >= tm_start.tm_min) {
        /* Accendo il dispositivo sotto... */
        status = 1;
        switch_child();
    } else if (tm_current.tm_hour >= tm_end.tm_hour && tm_current.tm_min >= tm_end.tm_min) {
        status = 0;
        switch_child();
    }
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
}

int main(int argc, char* argv[]) {
    /* argv = [./timer, indice, /tmp/indice]; */
    char tmp[MAX_BUF_SIZE];       /* Buffer per le pipe*/
    char ppid_pipe[MAX_BUF_SIZE]; /* Pipe per il padre*/
    char* this_pipe = NULL;       /* Pipe di questo dispositivo */

    char** vars = NULL;
    int ppid, ppid_pipe_fd, mode;

    this_pipe = argv[2];
    pid = getpid();
    __index = atoi(argv[1]);
    fd = open(this_pipe, O_RDWR);

    shellpid = get_shell_pid();

    signal(SIGTERM, sighandler_int);
    signal(SIGUSR1, sighandler_int);
    signal(SIGUSR2, sighandler_int);

    while (1) {
        if (flag_usr1) {
            flag_usr1 = 0;
            sprintf(tmp, "5|%d|%d|%d|%d|%d|%d|%d|<!",
                    pid, __index, status,
                    tm_start.tm_hour, tm_start.tm_min,
                    tm_end.tm_hour, tm_end.tm_min);
            if (children_pids[0] != -1) {
                char* raw_info = get_raw_device_info(children_pids[0]);
                if (raw_info != NULL) {
                    strcat(tmp, raw_info);
                    strcat(tmp, "|!|");
                    free(raw_info);
                }
            }
            strcat(tmp, "!>");
            write(fd, tmp, MAX_BUF_SIZE);
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
            } else if (mode == 1) {
                char* shifted_tmp = malloc(MAX_BUF_SIZE * sizeof(shifted_tmp));
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
            if ((int)getppid() != shellpid) {
                ppid = (int)getppid();
                kill(ppid, SIGUSR2);
                get_pipe_name(ppid, ppid_pipe); /* Nome della pipe */
                ppid_pipe_fd = open(ppid_pipe, O_RDWR);
                sprintf(tmp, "2|%d", (int)getpid());
                write(ppid_pipe_fd, tmp, sizeof(tmp));
                close(ppid_pipe_fd);
            }
            exit(0);
        }
        check_time();
        sleep(10);
    }

    return 0;
}
