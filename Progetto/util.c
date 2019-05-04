#include "util.h"

char **split(char *__buf, int __count) {
    char *tokenizer = strtok(__buf, "|");
    char **vars = malloc(__count * sizeof(char *));
    int j = 0;

    while (tokenizer != NULL && j < __count) {
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
