#include "../actions.h"
#include "../util.h"

/* TIMER = 5 */

int shellpid;

/* Registri della lampadina */
int fd;           /* file descriptor della pipe verso il padre */
int pid, __index; /* variabili di stato */
int status = 0;   /* interruttore accensione */

struct tm tm_start;
struct tm tm_end;
struct tm tm_current;

volatile int flag_usr1 = 0;
volatile int flag_usr2 = 0;
volatile int flag_term = 0;

void check_time() {
    tm_current = *localtime(&(time_t){time(NULL)});
    if (tm_current.tm_hour >= tm_start.tm_hour && tm_current.tm_min >= tm_start.tm_min) {
        /* Accendo il dispositivo sotto... */
        status = 1;
    } else if (tm_current.tm_hour >= tm_end.tm_hour && tm_current.tm_min >= tm_end.tm_min) {
        status = 0;
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
            sprintf(tmp, "5|%d|%d|%d|%d|%d|%d|%d",
                    pid, __index, status,
                    tm_start.tm_hour, tm_start.tm_min,
                    tm_end.tm_hour, tm_end.tm_min);
            write(fd, tmp, MAX_BUF_SIZE);
        }
        if (flag_usr2) {
            flag_usr2 = 0;
            /* 
                Al ricevimento del segnale, la finestra apre la pipe in lettura e ottiene cosa deve fare.
                0|ORA|MINUTI|ORAFINE|MINUTIFINE -> imposta timer
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
