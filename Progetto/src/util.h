#ifndef UTIL_H
#define UTIL_H

#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_BUF_SIZE 1024

#define CONTROLLER 0
#define BULB 1
#define FRIDGE 2
#define WINDOW 3

#define BULB_S "1"
#define FRIDGE_S "2"
#define WINDOW_S "3"

#define BULB_PARAMETERS 5
#define FRIDGE_PARAMETERS 8
#define WINDOW_PARAMETERS 5

#define MAX_CHILDREN 10

#define SHELL_POSITION "bin/shell"
#define DEVICES_POSITIONS "bin/devices/"
#define PIPES_POSITIONS "/tmp/ipc/"
#define SHPM "/tmp/myshpm"

#define SWITCH_STRING \
    "Sintassi: switch <id> <label> <pos>\n\
    Interruttori disponibili: bulb: accensione, fridge: temperatura/apertura, window: apertura\n"

#define ADD_STRING \
    "Sintassi: add <device>\nDispositivi disponibili: bulb, window, fridge, hub, timer\n"

#define DEL_STRING \
    "Sintassi del <device>\n"

// structure for message queue
struct mesg_buffer {
    long mesg_type;
    char mesg_text[MAX_BUF_SIZE];
} message;

char *getUserName();
char **split(char *__buf);
char **split_fixed(char *__buf, int __count);
char *pipename(int pid);

int get_device_pid(int in, int *children_pids);
void get_device_name(int device_type, char* buf);

void __switch(char buf[][MAX_BUF_SIZE], int *children_pids);
void __info(char buf[][MAX_BUF_SIZE], int *children_pids);
#endif