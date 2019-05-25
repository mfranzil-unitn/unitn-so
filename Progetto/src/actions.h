#ifndef ACTIONS_H
#define ACTIONS_H

#include "util.h"

void __switch_index(int index, char *action, char *position, int *children_pids);
void __switch(int pid, char* action, char* position, char* device_info);
void __info(int index, int *children_pids);
void __list(int *children_pids);
void __print(char **vars);
int __add(char *device, int device_index, int *children_pids, const int devices_number, char *__out_buf);
void __del(int index, int *children_pids, char *__out_buf);
void __link(int index1, int index2, int *children_pids);
int __add_ex(char **vars, int *children_pids, const int devices_number);
int __link_ex(int son_pid, int parent_pid, int shellpid);

#endif
