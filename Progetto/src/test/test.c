#include <string.h>
#include "../util.h"

void __print(char **vars) {
    if (strcmp(vars[0], BULB_S) == 0) {
        // Lampadina - parametri: tipo, pid, indice, stato, tempo di accensione
        printf("Oggetto: Lampadina, PID: %s, Indice: %s, Stato: %s, Tempo di accensione: %s",
               vars[1], vars[2], atoi(vars[3]) ? "ON" : "OFF", vars[4]);
    } else if (strcmp(vars[0], FRIDGE_S) == 0) {
        // Frigo -  parametri: tipo, pid, indice, stato, tempo di apertura, delay
        // percentuale riempimento, temperatura interna
        printf("Oggetto: Frigorifero, Messaggio di log: <%s>, ", vars[8]);
        printf("PID: %s, Indice: %s, Stato: %s, Tempo di apertura: %s sec, ",
               vars[1], vars[2], atoi(vars[3]) ? "Aperto" : "Chiuso", vars[4]);
        printf("Delay richiusura: %s sec, Percentuale riempimento: %s, Temperatura: %s°C",
               vars[5], vars[6], vars[7]);
    } else if (strcmp(vars[0], WINDOW_S) == 0) {
        // Finestra - parametri: tipo, pid, indice, stato, tempo di accensione
        printf("Oggetto: Finestra, PID: %s, Indice: %s, Stato: %s, Tempo di apertura: %s sec",
               vars[1], vars[2], atoi(vars[3]) ? "Aperto" : "Chiuso", vars[4]);
    } else if (strcmp(vars[0], HUB_S) == 0) {
        printf("Oggetto: Hub, PID: %s, Indice: %s, Stato: %s, Dispositivi collegati: %s",
               vars[1], vars[2], atoi(vars[3]) ? "Acceso" : "Spento", vars[4]);
    } else {
        printf("Dispositivo non supportato.\n");
    }
}

char **Nsplit_fixed(char *__buf, int __count) {
    char *tokenizer = strtok(__buf, "|");
    char *old = NULL;
    int device = 0;

    //printf("Level %d\n", device);

    char **vars = malloc((10) * sizeof(*vars));
    int i = 0;

    while (tokenizer != NULL) {
        int j;
        if (strcmp(tokenizer, "<!") == 0) {
            ++device;  //   printf("\nLevel %d\n", ++device);
            __print(vars);
            i = 0;
            printf("\n");
            for (int j = 0; j < device; j++) {
                printf("    ");
            }
        } else if (strcmp(tokenizer, "!>") == 0) {
            --device;  //   printf("\nLevel %d\n", --device);
            if (strcmp(old, "<!") != 0) {
                __print(vars);
            }
        } else if (strcmp(tokenizer, "!") == 0) {
            if (strcmp(old, "!>") != 0) {
                __print(vars);
            }
            i = 0;
            printf("\n");
            for (int j = 0; j < device; j++) {
                printf("    ");
            }
        } else {
            vars[i++] = tokenizer;
            //printf("%s, ", tokenizer);
        }
        old = tokenizer;
        tokenizer = strtok(NULL, "|");
    }

    //printf("\nRemainder: %s", __buf);

    return NULL;
}
char **Nsplit(char *__buf) {
    // Divide una stringa presa dalla pipe
    // a seconda del dispositivo.

    // La prima cifra di __buf è sempre il tipo di dispositivo.

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

    return Nsplit_fixed(__buf, __count);
}

int main() {
    char buf[MAX_BUF_SIZE];

    while (1) {
        printf("\n> ");
        scanf("%s", buf);
        Nsplit(buf);
    }
}