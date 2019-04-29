#include <stdio.h>   // for printf
#include <stdlib.h>  // for srand, rand, wait
#include <time.h>    // for time
#include <unistd.h>  // for fork, getpid, getppid()

void delay(double dly) {
    const time_t start = time(NULL);
    time_t current;
    do {
        time(&current);
    } while (difftime(current, start) < dly);
}

int rnd(int min, int max) {
    srand(time(NULL) + getpid());  // init rand's seed
    int r;
    // will hold result
    int steps = (max - min) + 1;  // how many
    r = rand() % (steps);
    r += min;
    return (r);
}

int main() {
    pid_t fid;
    int r;
    fid = fork();
    if (fid == -1) {
        printf("?Error. Forking failed!\n");
        return (1);
    };
   // r = rnd(1, 6);
    delay(1);
    if (fid > 0) {
        delay(rnd(1, 6));
        printf("(waited for %d secs) Parent, My pid is %d. Generated child has pid=%d\n", r, getpid(), fid);
    } else {
        delay(rnd(1, 6));
        printf("(waited for %d secs) Child. My pid is %d. Parent of me has pid=%d\n", r, getpid(), getppid());
    };
    wait(NULL);  // wait for all children (parent has one, child has none)
    
    return (0);
}