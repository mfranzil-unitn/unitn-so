#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>

int main()

{
    key_t key;
    int i, a;
    int shm, shm1;
    char *addr, *addr1;
    struct shmid_ds buf;

    key = ftok("pathname", 3);
    printf("key=%d\n", key);

    shm1 = shmget(key, 100, IPC_CREAT + S_IRUSR + S_IWUSR);
    addr1 = shmat(shm1, NULL, 0);
    sprintf(addr1, "P3 wrote cccccccccccccccccccccccccccccccccccccccccccc");

    printf("P3: identifier of the shared memory shm1= %d\n", shm1);

    shmdt(addr1);
    /*
   shmctl(shm1,IPC_RMID,0); 
*/
}
