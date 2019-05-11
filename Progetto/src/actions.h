#ifndef ACTIONS_H
#define ACTIONS_H

#include "util.h"

void __switch(int device, char *action, char *position, int *children_pids);

void __info(int index, int *children_pids);
void __list(int *children_pids);

int __add(char *device, int device_index, int actual_index, int *children_pids, char *__out_buf);
void __del(int index, int *children_pids, char *__out_buf);

void __link(int index1, int index2, int *children_pids);

#endif