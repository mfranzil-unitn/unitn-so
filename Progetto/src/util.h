#ifndef UTIL_H
#define UTIL_H

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define MAX_BUF_SIZE 1024

#define CONTROLLER 0
#define BULB 1
#define FRIDGE 2
#define WINDOW 3
#define HUB 4

#define BULB_S "1"
#define FRIDGE_S "2"
#define WINDOW_S "3"
#define HUB_S "4"

#define BULB_PARAMETERS 5
#define FRIDGE_PARAMETERS 9
#define WINDOW_PARAMETERS 5
#define HUB_PARAMETERS 5000

#define MAX_CHILDREN 20

#define SHELL_POSITION "bin/shell"
#define DEVICES_POSITIONS "bin/devices/"
#define PIPES_POSITIONS "/tmp/ipc/"
#define SHPM "/tmp/myshpm"

#define SWITCH_STRING \
    "Sintassi: switch <id> <label> <pos>\n\
    Interruttori disponibili:\n        bulb: accensione\n        fridge: temperatura/apertura/delay/riempimento\n        window: apertura\n"

#define ADD_STRING \
    "Sintassi: add <device>\nDispositivi disponibili: bulb, window, fridge, hub, timer\n"

#define DEL_STRING \
    "Sintassi del <device>\n"

#define INFO_STRING \
    "Sintassi: info <device>\n"

#define LIST_STRING \
    "Sintassi: list\n"

#define LINK_STRING \
    "Sintassi: link <id> to <controller>\n"

// structure for message queue
struct mesg_buffer {
    long mesg_type;
    char mesg_text[MAX_BUF_SIZE];
} message;

void lprintf(const char *__restrict__ __format, ...);

int parse(char buf[][MAX_BUF_SIZE], int cmd_n);

char **split(char *__buf);
char **split_fixed(char *__buf, int __count);

char *get_shell_text();
void get_pipe_name(int pid, char *pipe_str);

int get_device_pid(int device_identifier, int *children_pids, char **raw_info);
void get_device_name(int device_type, char *buf);
void get_device_name_str(char *device_type, char *buf);
int get_shell_pid();

char *get_raw_device_info(int pid);
/*char **get_device_info(int pid);*/

int is_controller(int pid, char *raw_info);
int hub_is_full(int pid, char *raw_info);

void hub_tree_print(char **vars);
void hub_tree_spaces(int level);
void hub_tree_parser(char *__buf);
int hub_tree_pid_finder(char *__buf, int id);
char **split_sons(char *__buf, int __count);

void change_ext_pid(int newpid);
#endif
