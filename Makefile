CC = clang
CFLAGS = -Wall -Wextra -Werror -Wpedantic -g

all: httpserver

httpserver: httpserver_asgn4.o talk.o parser.o process.o io.o queue.o
	$(CC) -o httpserver $^ -lpthread
 
httpserver_asgn4.o: httpserver_asgn4.c
	$(CC) $(CFLAGS) -c httpserver_asgn4.c

talk.o: talk.c
	$(CC) $(CFLAGS) -c talk.c

parser.o: parser.c
	$(CC) $(CFLAGS) -c parser.c

process.o: process.c
	$(CC) $(CFLAGS) -c process.c

io.o: io.c
	$(CC) $(CFLAGS) -c io.c

queue.o: queue.c
	$(CC) $(CFLAGS) -c queue.c



test: test.o queue.o
	$(CC) $(CFLAGS) -o test $^

test.o: test.c
	$(CC) $(CFLAGS) -c test.c

clean:
	rm -f httpserver test *.o

format:
	clang-format -i -style=file *.[c,h]