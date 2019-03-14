#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>

int main() {
    key_t key;
    int i, a;
    int shm, shm1;
    char *addr, *addr1;
    struct shmid_ds buf;

    key = ftok("pathname", 3);
    printf("key=%d\n", key);

    shm1 = shmget(key, 100, IPC_CREAT + S_IRUSR + S_IWUSR);
    addr1 = shmat(shm1, NULL, 0);

    addr1 = shmat(shm1, NULL, 0);

    printf("P4: identifier of the shared memory shm1= %d\n", shm1);
    printf("P4 read from shared memory %s\n", addr1);
    sprintf(addr1, " P4 wrote on shared memory: bruno crispo");
    printf("%s\n", addr1);

    shmdt(addr1);
    shmctl(shm1, IPC_RMID, 0);
}
