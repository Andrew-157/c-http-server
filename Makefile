.PHONY: clean

run.out: run.o server.o
	gcc server.o run.o -o run.out

run.o: run.c
	gcc -c -Wall run.c -o run.o

server.o: server.c
	gcc -c -Wall server.c -o server.o

clean:
	rm -vf *.out *.o
