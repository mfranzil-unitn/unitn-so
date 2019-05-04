#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <string.h>

#define MAX_BUF_SIZE 1024

char* getUserName();
char **split(char *__buf, int __count);

#endif