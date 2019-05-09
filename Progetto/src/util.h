#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <string.h>

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

#define MAX_CHILDREN 20

#define SHELL_POSITION "bin/shell"
#define DEVICES_POSITIONS "bin/devices/"
#define PIPES_POSITIONS "/tmp/ipc/"
#define SHPM "/tmp/myshpm"

char* getUserName();
char **split(char *__buf);
char **split_fixed(char *__buf, int __count);

#endif