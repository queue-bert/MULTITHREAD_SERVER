CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -std=c99 -g
LDLIBS = -lpthread

server: queue.o server.o util.o
	$(CC) $(CFLAGS) $(LDLIBS) $^ -o $@

server.o: server.c util.h queue.h
	$(CC) $(CFLAGS) -c $< -o $@

util.o: util.c util.h
	$(CC) $(CFLAGS) -c $< -o $@

queue.o: queue.c queue.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f server *.o

run: server
	gnome-terminal -- bash -c "./server 8080; exec bash"

debug: server
	gnome-terminal -- bash -c "./server 8080; exec bash"

push:
ifndef COMMIT_MSG
	$(error COMMIT_MSG is not set)
endif
	git add .
	git commit -m "$(COMMIT_MSG)"
	git push
