.PHONY: clean

server.out: server.c
	gcc -Wall -o server.out server.c

clean:
	rm -vf *.out *.o
