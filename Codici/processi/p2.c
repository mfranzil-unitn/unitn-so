#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>

int main() {
    key_t key;
    int i, a;
    int shm;
    char *addr;
    struct shmid_ds buf;

    key = 8888;
    shm = shmget(key, 1000, S_IRUSR + S_IWUSR);
    addr = shmat(shm, NULL, 0);
    strncat(addr, "P2 added this content to shared memory", 1000 - (strlen(addr) + 1));

    printf("P2: identifier of the shared memory shm= %d\n", shm);
    printf("P2: address of the shared memory addr= %d\n", *addr);
    printf(" %s %d\n", addr, shm);

    /*
    shmdt(addr(;
    shmctl(shm, IPC_RMID,0);
*/
}
