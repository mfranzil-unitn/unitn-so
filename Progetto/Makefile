CC = gcc
CFLAGS = -std=gnu90 --pedantic

main: build 

build: bin/shell bin/bulb bin/fridge bin/window
	rm -f -r /tmp/ipc
	mkdir /tmp/ipc
	rm -f ./run
	ln -s ./bin/shell ./run
	chmod +x run

launcher: util shell launcher.c
	$(CC) $(CFLAGS) -o launcher launcher.c bin/util.o

bin/shell: src/shell.c bin/util.o
	$(CC) $(CFLAGS) -o bin/shell src/shell.c bin/util.o

bin/bulb: src/devices/bulb.c bin/util.o
	$(CC) $(CFLAGS) -o bin/bulb src/devices/bulb.c bin/util.o

bin/fridge: src/devices/fridge.c bin/util.o
	$(CC) $(CFLAGS) -o bin/fridge src/devices/fridge.c bin/util.o

bin/window: src/devices/window.c bin/util.o
	$(CC) $(CFLAGS) -o bin/window src/devices/window.c bin/util.o

bin/util.o: src/util.c
	$(CC) $(CFLAGS) -c -o bin/util.o src/util.c 
