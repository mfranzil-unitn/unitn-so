#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
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

    /* Le seguenti variabili sono usate in usr2 */
    char* mall_tmp = NULL; /* Viene usato al posto di tmp in USR2, richiede malloc */
    char* raw_info = NULL;
    int over_index[MAX_CHILDREN];
    int code;
    char** vars;

    int i;

    int connected=0;

    this_pipe = argv[2];
    pid = getpid();
    __index = atoi(argv[1]);
    fd = open(this_pipe, O_RDWR);

    for (i = 0; i < MAX_CHILDREN; i++) {
        children_pids[i] = -1;
    }

    key = ftok("/tmp/ipc/mqueues",__index);
    msgid = msgget(key, 0666 | IPC_CREAT);
    
        read_msgqueue(msgid, children_pids);
        //printf("HI BRO I'M HUB %d\n", __index);
        for (i = 0; i < MAX_CHILDREN; i++) {
                if (children_pids[i] != -1) {
                    //printf("Dispositivo %d: %d\n", i, children_pids[i]);
                }
            }
    shellpid = get_shell_pid();

    key_pid = ftok("/tmp/ipc/mqueues", pid) ;
    msgid_pid = msgget(key_pid, 0666 | IPC_CREAT);
    //printf(" KEY %d MSGID: %d\n",key_pid, msgid_pid);

    signal(SIGCHLD, SIG_IGN);
    signal(SIGTERM, sighandler_int);
    signal(SIGUSR1, sighandler_int);
    signal(SIGUSR2, sighandler_int);


    while (1) {
           __index = atoi(argv[1]);
            
        if (flag_usr1) {
            flag_usr1 = 0;
            //printf("hub usr1: %d\n", pid);
            /* bisogna controllare se i dispositivi sono allineati o meno (override) */

            /* conto i dispositivi connessi */
            connected = 0;

           

            for (i = 0; i < MAX_CHILDREN; i++) {
                if (children_pids[i] != -1) {
                    //printf("Children pids[%d]: %d\n", i, children_pids[i] );
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

            //write(fd, tmp, MAX_BUF_SIZE);
            //printf("INFO SENT\n");

             sprintf(tmp, "4|%d|%d|%d|%d|<!|",
                    pid, __index, status, connected);
            
            
            /* Stampo nel buffer tante volte quanti device ho */
            
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
            message.mesg_type = 1;
            sprintf(message.mesg_text, "%s", tmp);
            //printf("HUB message: %s\n", message.mesg_text);
            msgsnd(msgid_pid, &message, sizeof(message), 0);
            //printf("MESSAGE SENT %s\n", message.mesg_text);
        }
        if (flag_usr2) {
            flag_usr2 = 0;
            /* La finestra apre la pipe in lettura e ottiene cosa deve fare. */
            /* 0|.. -> spegni/accendi tutto */
            /* 1|.. -> attacca contenuto */
            /* 2|.. -> toglie contenuto */

            printf("hub usr2: %d\n", pid);

            mall_tmp = malloc(MAX_BUF_SIZE * sizeof(mall_tmp));
            /*read(fd, mall_tmp, MAX_BUF_SIZE);
            printf("End Read: %s\n\n", mall_tmp);*/
            msgrcv(msgid_pid, &message, sizeof(message), 1, 0);
            sprintf(mall_tmp, "%s", message.mesg_text);
            printf("End Read: %s\n\n", mall_tmp);
            code = mall_tmp[0] - '0';
            /*printf("hub code: %d\n", code); */

            for (i = 0; i < MAX_CHILDREN; i++) {
                over_index[i] = 0;
            }

            /*Valore che indica lo stato di override o meno. Al MOMENTO INCARTAT TUTTO BOIA. */
            /*override = check_override(over_index); */

            if (code == 0) {
                /*printf("CODE 0\n"); */
                status = !status;
                for (i = 0; i < MAX_CHILDREN; i++) {
                    if (children_pids[i] != -1 && !over_index[i]) {
                        __switch(children_pids[i], "accensione", status ? "off" : "on", children_pids);
                    }
                }
                free(mall_tmp);
            }
            if (code == 1) {
                //printf("CODE 1\n");
                mall_tmp = mall_tmp + 2;
                //printf("\n\nMALL TMP FOR HUB %d: %s\n\n",__index, mall_tmp);
                vars = split(mall_tmp);
                __add_ex(vars, children_pids);
                free(vars);
                free(mall_tmp - 2);
            }
            if (code == 2) {
                /*printf("CODE 2\n"); */
                vars = split(mall_tmp);
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
    int ppid = (int)getppid();
    int fd, i;
    char pipe_str[MAX_BUF_SIZE];
    char tmp[MAX_BUF_SIZE];
    int ret;

    //printf("IN TERM FOR HUB: %d\n", __index);
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
    int count = 0;
    sprintf(tmp, "-");
    for (i = 0; i < MAX_CHILDREN; i++) {
        if (children_pids[i] != -1) {
            //printf("Trying to send pids\n");
            count++;
            //printf("Trying to get INFO\n");
            char* info = get_raw_device_info(children_pids[i]);
            //printf("INFO WE HAVE!: %s\n", info);
            char intern[MAX_BUF_SIZE];
            sprintf(intern, "-%s", info);
            //printf("INTERN: %s\n", intern);
            strcat(tmp, intern);
            kill(children_pids[i], SIGTERM);
        }
    }
    message.mesg_type = 1;
    sprintf(message.mesg_text, "%d%s", count, tmp);
    msgsnd(msgid, &message, sizeof(message), 0);

    //int ret = __link_ex(children_pids, ppid, shellpid);


    if (done) {
        exit(0);
    } else {
        printf("Errore nell'eliminazione\n");
    }
}


void read_msgqueue(int msgid, int* device_pids) {
    int n_devices;
    int ret = msgrcv(msgid, &message, sizeof(message), 1, IPC_NOWAIT);
    if (ret != -1) {
        int q = 0;
        char n_dev_str[100];
        while (!(message.mesg_text[q] == '-')) {
            n_dev_str[q] = message.mesg_text[q];
            q++;
        }
        n_dev_str[q] = '\0';
        n_devices = atoi(n_dev_str);
        if (n_devices > 0) {
            int __count = n_devices;
            char tmp_buf[MAX_BUF_SIZE];
            sprintf(tmp_buf, "%s", message.mesg_text);
            char** vars = NULL;
            vars = split_sons(tmp_buf, __count);
            int j = 0;
            while (j <= __count) {
                if (j >= 1) {
                    //printf("Vars %d: %s\n", j, vars[j]);
                    char** son_j = split(vars[j]);
                    __add_ex(son_j, children_pids);
                    //printf("ADD_EX GOOD\n");
                }
                j++;
            }
        }
    }
}