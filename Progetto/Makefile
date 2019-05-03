CC = gcc
CFLAGS = -std=gnu90

main: build 

build: shell launcher bulb fridge window

launcher: util shell
	$(CC) $(CFLAGS) -o launcher launcher.c bin/util.o

shell: util
	$(CC) $(CFLAGS) -o bin/shell shell.c bin/util.o

bulb:
	$(CC) $(CFLAGS) -o bin/bulb devices/bulb.c

fridge:
	$(CC) $(CFLAGS) -o bin/fridge devices/fridge.c

window: util
	$(CC) $(CFLAGS) -o bin/window devices/window.c bin/util.o

util:
	$(CC) $(CFLAGS) -c util.c -o bin/util.o