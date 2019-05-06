CC = gcc
CFLAGS = -std=gnu90 --pedantic

main: build 

build: shell bulb fridge window
	rm -f -r /tmp/ipc
	mkdir /tmp/ipc
	rm -f ./run
	ln -s ./bin/shell ./run
	chmod +x run

launcher: util shell launcher.c
	$(CC) $(CFLAGS) -o launcher launcher.c bin/util.o

shell: shell.c util
	$(CC) $(CFLAGS) -o bin/shell shell.c bin/util.o

bulb: devices/bulb.c util
	$(CC) $(CFLAGS) -o bin/bulb devices/bulb.c bin/util.o

fridge: devices/fridge.c util
	$(CC) $(CFLAGS) -o bin/fridge devices/fridge.c bin/util.o

window: devices/window.c util
	$(CC) $(CFLAGS) -o bin/window devices/window.c bin/util.o

util: util.c
	$(CC) $(CFLAGS) -c -o bin/util.o util.c 
