CC = gcc
CFLAGS=-g -Wall -Wuninitialized -std=c11
OUTPUT = spacerace

all : main.o spaceship.o control_center.o display.o
	$(CC) $(CFLAGS) -o $(OUTPUT) main.o spaceship.o control_center.o display.o

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

spaceship.o: spaceship.c spaceship.h
	$(CC) $(CFLAGS) -c spaceship.c

control_center.o: control_center.c spaceship.h
	$(CC) $(CFLAGS) -c control_center.c

display.o: display.c display.h
	$(CC) $(CFLAGS) -c display.c

clean:
	rm -f *.o $(OUTPUT)
