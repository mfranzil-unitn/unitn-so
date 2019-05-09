CC = gcc
CFLAGS = -std=gnu90 --pedantic

main: build 

build: bin/launcher bin/shell bin/devices/bulb bin/devices/fridge bin/devices/window
	rm -f -r /tmp/ipc
	mkdir /tmp/ipc
	rm -f ./run
	ln -s ./bin/launcher ./run
	chmod +x run

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
