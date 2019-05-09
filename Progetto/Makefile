.PHONY: build clean help targets

CC = gcc
CFLAGS = -std=gnu90
CPEDANTIC = -std=gnu90 --pedantic

BOLD_RED=\033[1;31m
BOLD_GREEN=\033[1;32m
GREEN=\033[0;32m
PINK=\033[0;35m
BOLD_PINK=\033[1;35m
CYAN=\033[0;36m
BOLD_CYAN=\033[1;36m
NC=\033[0m

default: help

targets: bin/launcher bin/shell bin/devices/bulb bin/devices/fridge bin/devices/window

bin/launcher: src/launcher.c bin/util.o
	$(CC) $(CFLAGS) -o bin/launcher src/launcher.c bin/util.o

bin/shell: src/shell.c bin/util.o
	$(CC) $(CFLAGS) -o bin/shell src/shell.c bin/util.o

bin/devices/bulb: src/devices/bulb.c bin/util.o
	$(CC) $(CFLAGS) -o bin/devices/bulb src/devices/bulb.c bin/util.o

bin/devices/fridge: src/devices/fridge.c bin/util.o
	$(CC) $(CFLAGS) -o bin/devices/fridge src/devices/fridge.c bin/util.o

bin/devices/window: src/devices/window.c bin/util.o
	$(CC) $(CFLAGS) -o bin/devices/window src/devices/window.c bin/util.o

bin/util.o: src/util.c
	$(CC) $(CFLAGS) -c -o bin/util.o src/util.c 

clean:
	@rm -f -r /tmp/ipc
	@rm -f -r bin
	@rm -f run
	@echo "Pulizia completa."

build:
	@make clean
	@mkdir /tmp/ipc
	@mkdir bin
	@mkdir bin/devices
	@make targets
	@ln -s bin/launcher run
	@chmod +x run
	@echo "Compilazione completata. Usa ./run per avviare il launcher."

help: 
	@echo "$(BOLD_CYAN)INFORMAZIONI SUL PROGETTO$(NC)"
	@echo "$(GREEN)Realizzato da:$(NC)"
	@echo "\tMatteo Franzil"
	@echo "\tPaolo Baiguera"
	@echo "\tRuben Bettoni"
	@echo "\nPer iniziare, digita $(RED)make build$(NC) e poi $(RED)./run$(NC), oppure $(RED)make clean$(NC) per rimuovere i file temporanei."