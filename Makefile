CC=gcc
CFLAGS= -std=gnu11


all: solarmon

solarmon: solarmon.o
	$(CC) -o solarmon $< $(CFLAGS)


.PHONY : clean

clean:
	rm -f solarmon solarmon.o

