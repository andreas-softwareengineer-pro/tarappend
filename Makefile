all: tarappend

tarappend: tarappend.c tarheader.h
	gcc -o $@ tarappend.c

