#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>

int *shared;
int shmid, pid;

int main() {
    int s, i;

    shmid = shmget(IPC_PRIVATE, sizeof(int), 0600);
    printf("shmid=%d\n", shmid);

    shared = shmat(shmid, 0, 0);
    *shared = 100;

    printf("%d\n", *shared);
    pid = fork();
    if (pid == 0)  /* son */
    {
        *shared = 1000;
        shmdt((const void *)shared);
    } else  /* father */
    {
        wait(&s);
        printf("%d\n", *shared);
        shmdt((const void *)shared);
        shmctl(shmid, IPC_RMID, 0);
    }
    return 0;
}
