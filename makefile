all: main.c
	gcc main.c -o bank -lpthread

clean:
	rm bank
