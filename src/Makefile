server.out: logger.o server.o # *.out because .gitignore
	cc logger.o server.o -o server.out

logger.o: logger.c
	gcc -c -Wall logger.c -o logger.o

server.o: server.c
	cc -c -Wall server.c -o server.o

clean:
	rm -f *o *out
