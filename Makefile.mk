CC = gcc
CFLAGS = -std=c99 -Wall -O2

all: altitude_streamer altitude_receiver

altitude_streamer: altitude_streamer.c
	$(CC) $(CFLAGS) -o altitude_streamer altitude_streamer.c

altitude_receiver: altitude_receiver.c
	$(CC) $(CFLAGS) -o altitude_receiver altitude_receiver.c

clean:
	rm -f altitude_streamer altitude_receiver *.o
