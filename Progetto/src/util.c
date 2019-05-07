#include "util.h"

char** split(char* __buf) {
    // Divide una stringa presa dalla pipe
    // a seconda del dispositivo.

    // La prima cifra di __buf è sempre il tipo di dispositivo.

    int device = __buf[0] - '0';
    int __count;

    switch(device) {
        case BULB:
            __count = BULB_PARAMETERS;
            break;
        case FRIDGE:
            __count = FRIDGE_PARAMETERS;
            break;
        case WINDOW:
            __count = WINDOW_PARAMETERS;
            break;
        default:
            __count = 1;
            break;
    }

    char *tokenizer = strtok(__buf, "|");
    char **vars = malloc(__count * sizeof(char *));
    int j = 0;

    while (tokenizer != NULL && j < __count) {
        vars[j++] = tokenizer;
        tokenizer = strtok(NULL, "|");
    }

    return vars;
}

char** split_fixed(char* __buf, int __count) {
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

