CC=gcc
CFLAGS=


all: solarmon

solarmon: solarmon.o
	$(CC) -o solarmon $< $(CFLAGS)


.PHONY : clean

clean:
	rm -f solarmon solarmon.o

