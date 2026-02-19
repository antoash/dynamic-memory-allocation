CC=gcc
DB=gdb

all: ifl

ifl: ifl.c
	$(CC) ifl.c -g -o ifl.out && ./ifl.out 

debug:
	$(DB) ./ifl.out 

clean: 
	rm -rf main.out ifl.out