#include "../actions.h"
#include "../util.h"

// HUB = 4
int fd;                // file descriptor della pipe verso il padre
char* pipe_fd = NULL;  // nome della pipe

int pid, __index;  // variabili di stato

int status = 0;  // interruttore accensione

int children_pids[MAX_CHILDREN];
int override = 0;
int shellpid;

void sighandle_sigterm(int signal) {
    int done = 1;
    int ppid = (int)getppid();
    /*if (ppid != shellpid) {
        kill(ppid, SIGUSR2);
        char pipe_str[MAX_BUF_SIZE];
        get_pipe_name(ppid, pipe_str);  // Nome della pipe
        int fd = open(pipe_str, O_RDWR);
        char tmp[MAX_BUF_SIZE];
        sprintf(tmp, "2|%d", (int)getpid());
        write(fd, tmp, sizeof(tmp));
    }*/

    int ret = __link_ex(children_pids, ppid, shellpid);
    if (done) {
        exit(0);
    } else {
        printf("Errore nell'eliminazione\n");
    }
}

void sighandle_usr1(int sig) {
    // bisogna controllare se i dispositivi sono allineati o meno (override)
    char buffer[MAX_BUF_SIZE];

    // conto i dispositivi connessi
    int connected = 0;

    int i;
    for (i = 0; i < MAX_CHILDREN; i++) {
        if (children_pids[i] != -1) {
            connected++;
        }
    }

    sprintf(buffer, "4|%i|%i|%i|%i|<!|",
            pid, __index, status, connected);

    // Stampo nel buffer tante volte quanti device ho
    for (i = 0; i < MAX_CHILDREN; i++) {
        if (children_pids[i] != -1) {
            char* raw_info = get_raw_device_info(children_pids[i]);
            if (raw_info == NULL) {
                printf("Errore di collegamento (PID: %d)\n", children_pids[i]);
                continue;
            } else {
                strcat(buffer, raw_info);
                strcat(buffer, "|!|");
                free(raw_info);
            }
        }
    }

    strcat(buffer, "!>");
    write(fd, buffer, MAX_BUF_SIZE);
}

//Itera sui figli, in realtà fino a MAX_CHILDREN, e controlla che gli stati siano congruenti.
//E modifica il vettore over_index Maschera di bit.
int check_override(int* over_index) {
    int i = 0;
    int ret = 0;
    for (i = 0; i < MAX_CHILDREN; i++) {
        if (children_pids[i] != -1) {
            char** vars = split(get_device_raw_info(children_pids[i]));
            if (atoi(vars[3]) != status) {
                over_index[i] = 1;
                ret = 1;
            }
        }
    }
    return ret;
}

void sighandle_usr2(int sig) {
    // Al ricevimento del segnale, la finestra apre la pipe in lettura e ottiene cosa deve fare.
    // 0|.. -> spegni/accendi tutto
    // 1|.. -> attacca contenuto
    // 2|.. -> toglie contenuto
    char* tmp = malloc(MAX_BUF_SIZE * sizeof(tmp));
    int over_index[MAX_CHILDREN];
    read(fd, tmp, MAX_BUF_SIZE);
    //printf("End Read: %s\n\n", tmp);
    int code = tmp[0] - '0';
    //printf("code: %d\n", code);

    int k = 0;
    for (k = 0; k < MAX_CHILDREN; k++) {
        over_index[k] = 0;
    }

    //Valore che indica lo stato di override o meno. Al MOMENTO INCARTAT TUTTO BOIA.
    override = check_override(over_index);
    for (k = 0; k < MAX_CHILDREN; k++) {
    }

    if (code == 0) {
        //printf("CODE 0\n");
        status = !status;
        int i = 0;
        char* pipe_str;
        for (i = 0; i < MAX_CHILDREN; i++) {
            if (children_pids[i] != -1 && !over_index[i]) {
                char* pos = "on";
                if (status) {
                    pos = "off";
                }
                __switch(children_pids[i], "accensione", pos, children_pids);
            }
        }
        free(tmp);
    }
    if (code == 1) {
        //printf("CODE 1\n");
        tmp = tmp + 2;
        char** vars = split(tmp);
        __add_ex(vars, children_pids);
        free(vars);
        free(tmp - 2);
    }
    if (code == 2) {
        //printf("CODE 2\n");
        char** vars = split(tmp);
        int j = 0;
        for (j = 0; j < MAX_CHILDREN; j++) {
            if (children_pids[j] == atoi(vars[1])) {
                //printf("BECCATO: childern_Pids: %d, atoi: %d\n", children_pids[j], atoi(vars[1]));
                children_pids[j] = -1;
            }
        }
    }
}

int main(int argc, char* argv[]) {
    // argv = [./hub, indice, /tmp/pid];
    pipe_fd = argv[2];
    pid = getpid();
    __index = atoi(argv[1]);

    fd = open(pipe_fd, O_RDWR);

    int i;
    for (i = 0; i < MAX_CHILDREN; i++) {
        children_pids[i] = -1;
    }

    shellpid = get_shell_pid();

    signal(SIGTERM, sighandle_sigterm);
    signal(SIGUSR1, sighandle_usr1);
    signal(SIGUSR2, sighandle_usr2);
    signal(SIGCHLD, SIG_IGN);

    while (1) {
        ;
    }

    return 0;
}
