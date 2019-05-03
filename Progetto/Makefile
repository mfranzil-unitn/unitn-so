CC = gcc
CFLAGS = -std=gnu90

main: build 

build: shell bulb fridge window

shell: shell.c
	$(CC) $(CFLAGS) -o shell shell.c

bulb: bulb.c
	$(CC) $(CFLAGS) -o bulb bulb.c

fridge: fridge.c
	$(CC) $(CFLAGS) -o fridge fridge.c

window: window.c
	$(CC) $(CFLAGS) -o window window.c