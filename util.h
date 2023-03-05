#ifndef UTIL_H
#define UTIL_H

#define ACK 10
#define SOCKETERROR -1
#define PACKET BUFSIZE + HEADER
#define BUFFER BUFSIZE * 2
#define BACKLOG 64
#define POOL_THREADS 20
#define DOCUMENT_ROOT "./src"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/stat.h>
#include "queue.h"

int sendall(int s, char *buf, int *len);

int packetize(char* pdata, char* packet, int length, char mode);

int depacketize(char* packet, char* mode, uint16_t* length, char** data, int n_recv);

int get_fsize(char* file, struct stat st);

int check(int stat, char* message);

void * connect_and_send(void * client_socket_fd);

// void * thread_function(void * arg);
void * thread_function();

const char* get_mime_type(const char* file_path);

int rm_null(char * str, int n);

int valid_path(char * req_uri, char * filename, char * req_version, char * packet, int client_socket);

#endif