#include "util.h"
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

char **split(char *tmp, int count) {
    char *tokenizer = strtok(tmp, "|");
    char **vars = malloc(count * sizeof(char *));
    int j = 0;

    while (tokenizer != NULL) {
        vars[j++] = tokenizer;
        tokenizer = strtok(NULL, "|");
    }

    return vars;
}

char *getUserName() {
    int uid = geteuid();
    struct passwd *pw = getpwuid(uid);
    if (pw) {
        char hostname[MAX_BUF_SIZE];
        hostname[MAX_BUF_SIZE - 1] = '\0';
        gethostname(hostname, MAX_BUF_SIZE - 1);
        return strcat(strcat(pw->pw_name, "@"), hostname);
    }
    return "host";
}
