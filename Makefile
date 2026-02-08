CC=gcc

all: ifl

ifl: ifl.c
	$(CC) ifl.c -o ifl.out && ./ifl.out

clean: 
	rm -rf main.out ifl.out