#ifndef UTIL_H
#define UTIL_H

#define ACK 10
#define SOCKETERROR -1
#define PACKET BUFSIZE + HEADER
#define BUFFER BUFSIZE * 2
#define BACKLOG 64
#define POOL_THREADS 20
#define DOCUMENT_ROOT "./www"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <sys/stat.h>
#include "queue.h"

extern volatile sig_atomic_t flag;


int sendall(int s, char *buf, int *len);

int get_fsize(char* file, struct stat st);

int check(int stat, char* message);

void connect_and_send(int * client_socket_fd);

void * thread_function();

const char* get_mime_type(const char* file_path);

int valid_path(char * req_uri, char * filename);

#endif