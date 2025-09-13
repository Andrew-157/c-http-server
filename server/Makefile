server.out: logger.o request_handling.o server.o # *.out because .gitignore
	gcc logger.o request_handling.o server.o -o server.out

logger.o: logger.c
	gcc -c -Wall logger.c -o logger.o

request_handling.o: request_handling.c
	gcc -c -Wall request_handling.c -o request_handling.o

server.o: server.c
	gcc -c -Wall server.c -o server.o

clean:
	rm -f *o *out test.c
