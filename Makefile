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
	gnome-terminal -- bash -c "gdb -ex run --args ./server 8080; exec bash"

push:
ifndef COMMIT_MSG
	$(error COMMIT_MSG is not set)
endif
	git add .
	git commit -m "$(COMMIT_MSG)"
	git push


debugg: server
	export LD_LIBRARY_PATH=/usr/lib/debug:/lib/x86_64-linux-gnu/debug:/usr/lib/x86_64-linux-gnu/debug
	gnome-terminal -- bash -c "gdb --args ./server 1600 -ex 'set args 1600' -ex 'set env LD_PRELOAD /lib/x86_64-linux-gnu/libpthread.so.0' -ex 'set env LIBC_FATAL_STDERR_=1' -ex 'handle SIGPIPE nostop' -ex 'run' --debug-info-mismatch; exec bash"
