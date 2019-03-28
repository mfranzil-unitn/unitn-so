#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void) {
    time_t current_time;
    char* c_time_string;
    current_time = time(NULL);  // current time
    if (current_time == ((time_t)-1)) {
        (void)fprintf(stderr, "Failure to obtain the current time.\n");
        exit(EXIT_FAILURE);
    };
    c_time_string = ctime(&current_time);  // convert to readable format
    if (c_time_string == NULL) {
        (void)fprintf(stderr, "Failure to convert the current time.\n");
        exit(EXIT_FAILURE);
    };
    (void)printf("Current time is %s", c_time_string);  // newline already set
    exit(EXIT_SUCCESS);
}