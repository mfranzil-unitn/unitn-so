#include <string.h>
#include "../util.h"

void get_device_name(int device_type, char *buf) {
    switch (device_type) {
        case BULB:
            sprintf(buf, "lampadina");
            break;
        case FRIDGE:
            sprintf(buf, "frigo");
            break;
        case WINDOW:
            sprintf(buf, "finestra");
            break;
        case CONTROLLER:
            sprintf(buf, "centralina");
            break;
        case HUB:
            sprintf(buf, "hub");
            break;
        default:
            sprintf(buf, "-");
            break;
    }
}

void hub_tree_print(char **vars) {
    char device_name[MAX_BUF_SIZE];

    if (strcmp(vars[0], HUB_S) == 0) {
        printf("Hub (PID: %s, Indice: %s), Stato: %s, Collegati: %s",
               vars[1], vars[2], atoi(vars[3]) ? "Acceso" : "Spento", vars[4]);
    } else if (strcmp(vars[0], TIMER_S) == 0) {
        printf("Timer (PID: %s, Indice: %s), Stato: %s, Orari: %s:%s -> %s:%s\n, Collegati: %s",
               vars[1], vars[2], atoi(vars[3]) ? "Acceso" : "Spento", vars[4], vars[5], vars[6], vars[7], vars[8]);
    } else {
        get_device_name(atoi(vars[0]), device_name);
        device_name[0] += 'A' - 'a';

        printf("%s (PID %s, Indice %s)", device_name, vars[1], vars[2]);
    }
}

void hub_tree_spaces(int level) {
    int j;

    if (level > 0) {
        printf("\n");
        for (j = 0; j < level; j++) {
            printf("  ");
        }
        printf("∟ ");
    }
}

void hub_tree_parser(char *__buf) {
    char *tokenizer = strtok(__buf, "|");
    char *old = NULL;
    int level = 0;
    char **vars;
    int i = 0;
    int to_be_printed = 1;
    /* printf("Level_a %d\n", level); */

    vars = malloc((FRIDGE_PARAMETERS + 4) * sizeof(*vars));

    while (tokenizer != NULL) {
        /* printf("\nLevel %d tokenizer %s to_be_printed %d\n", level, tokenizer, to_be_printed); */
        if (strcmp(tokenizer, "<!") == 0) {
            to_be_printed += atoi(old) - 1;
            hub_tree_spaces(level);
            i = 0;
            printf("(%d, <!) ", level++);
            hub_tree_print(vars);
        } else if (strcmp(tokenizer, "!>") == 0) {
            --level; /*  printf("\nLevel_c %d\n", level); */

            if (strcmp(old, "<!") != 0 && strcmp(old, "!") != 0 && to_be_printed > 0) {
                i = 0;
                hub_tree_spaces(level);
                printf("(%d, !>) ", level);
                hub_tree_print(vars);
                to_be_printed--;
            }
        } else if (strcmp(tokenizer, "!") == 0) {
            if (strcmp(old, "!>") != 0 && to_be_printed > 0) {
                i = 0;
                hub_tree_spaces(level);
                printf("(%d, !) ", level);
                hub_tree_print(vars);
                to_be_printed--;
            }
        } else {
            vars[i++] = tokenizer;
            /*printf("%s, ", tokenizer); */
        }
        old = tokenizer;
        tokenizer = strtok(NULL, "|");
    }
    printf("\n");
    free(vars);
}

int hub_tree_pid_finder(char *__buf, int id, char **raw_info) {
    char *tokenizer = strtok(__buf, "|");
    char *old = NULL;
    char **vars;
    int i = 0, j;
    int children = 1;
    int level = 0;
    int pid_to_be_returned = -1;
    /*printf("Level %d\n", level); */

    /* DISPOSITIVO; PID; ID */

    int found_flag = -1;

    char *target;
    vars = malloc((FRIDGE_PARAMETERS + 4) * sizeof(*vars));

    while (tokenizer != NULL) {
        if (strcmp(tokenizer, "<!") == 0) {
            if (found_flag >= 0) {
                target += sprintf(target, "|<!");
            }
            children += atoi(old) - 1;
            level++;
            if (atoi(vars[2]) == id) {
                found_flag = level - 1;

                *raw_info = malloc(MAX_BUF_SIZE * sizeof(raw_info));
                target = *raw_info;

                pid_to_be_returned = atoi(vars[1]);
                target += sprintf(target, "%s", vars[0]);
                for (j = 1; j < i; j++) {
                    target += sprintf(target, "|%s", vars[j]);
                }
                target += sprintf(target, "|<!");
            }
            i = 0;
        } else if (strcmp(tokenizer, "!>") == 0) {
            level--;

            if (found_flag >= 0) {
                target += sprintf(target, "|!>");
            }

            if (found_flag == level) {
                printf("RETURN: %s\n", *raw_info);
                return pid_to_be_returned;
            }

            if (strcmp(old, "<!") != 0 && strcmp(old, "!") != 0 && children > 0) {
                i = 0;
                if (atoi(vars[2]) == id) {
                    return atoi(vars[1]);
                }
                children--;
            }
        } else if (strcmp(tokenizer, "!") == 0) {
            if (found_flag >= 0) {
                target += sprintf(target, "|!");
            }

            if (strcmp(old, "!>") != 0 && children > 0) {
                // CASO IN CUI E' FINITO UN FIGLIO SOLO
                if (atoi(vars[2]) == id) {
                    *raw_info = malloc(MAX_BUF_SIZE * sizeof(raw_info));
                    target = *raw_info;

                    target += sprintf(target, "%s", vars[0]);
                    printf("%s", vars[0]);
                    for (j = 1; j < i; j++) {
                        target += sprintf(target, "|%s", vars[j]);
                        printf("|%s", vars[j]);
                    }
                    return atoi(vars[1]);
                }
                i = 0;
                children--;
            }
        } else {
            vars[i++] = tokenizer;
            if (found_flag >= 0) {
                target += sprintf(target, "|%s", tokenizer);
            }
            /*printf("%s, ", tokenizer); */
        }
        old = tokenizer;
        tokenizer = strtok(NULL, "|");
    }
    free(vars);
    return -1;
}

/*

char **Nsplit(char *__buf) {
    int device = __buf[0] - '0';
    int __count;

    switch (device) {
        case BULB:
            __count = BULB_PARAMETERS;
            break;
        case FRIDGE:
            __count = FRIDGE_PARAMETERS;
            break;
        case WINDOW:
            __count = WINDOW_PARAMETERS;
            break;
        case HUB:
            __count = 5;
            break;
        default:
            __count = 1;
            break;
    }

    return hub_tree_parser(__buf, __count);
}
*/
int main() {
    /*time_t tempo = time(NULL);
    char *c = ctime(&tempo);
    printf("%d, %s\n", (int)tempo, c);

    struct tm tm_start = *localtime(&(time_t){time(NULL)});
    struct tm tm_end = *localtime(&(time_t){time(NULL)});

    scanf("%d:%d -> %d:%d",
          &tm_start.tm_hour, &tm_start.tm_min,
          &tm_end.tm_hour, &tm_end.tm_min);

    char buf[MAX_BUF_SIZE];

    strftime(buf, MAX_BUF_SIZE, "%H:%M", &tm_start);
    printf("Timer set from %s", buf);
    strftime(buf, MAX_BUF_SIZE, "%H:%M", &tm_end);
    printf(" to %s\n", buf);

    while (1) {
        time_t tim = time(NULL);
        if ((tim / 60) % 60 == tm_start.tm_min) {
            printf("WOW!\n");
            return;
        }
    }
*/
    /*printf("%d:%d\n", tempo % 60, tempo / 60); */

    char buf[MAX_BUF_SIZE];
    char buf2[MAX_BUF_SIZE];
    int d;
    while (1) {
        printf("String > ");
        scanf("%s", buf);
        strcpy(buf2, buf);
        hub_tree_parser(buf);
        printf("Index > ");
        scanf(" %d", &d);

        char *raw_info = NULL;
        int out = hub_tree_pid_finder(buf2, d, &raw_info);
        printf("PID %d, STRING %s\n", out, raw_info);
    }
}