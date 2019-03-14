#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>

int main()

{
    key_t key;
    int i, a;
    int shm;
    char *addr;
    struct shmid_ds buf;

    key = 8888;
    shm = shmget(key, 1000, IPC_CREAT + S_IRUSR + S_IWUSR);
    addr = shmat(shm, NULL, 0);
    sprintf(addr, "P1 wrote this content to shared memory %d ", shm);

    printf("P1: address of the shared memory addr= %d\n", *addr);
    printf("P1: identifier of the shared memory shm1= %d\n", shm);
    printf("%s\n", addr);

    /*
    shmdt(addr);
    shmctl(shm,IPC_RMID,0);
*/
}
