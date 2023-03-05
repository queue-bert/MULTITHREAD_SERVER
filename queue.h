#ifndef QUEUE_H
#define QUEUE_H

#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>

#define QUEUE_SIZE 100
#define BUFSIZE 512
#define HEADER 3


extern pthread_mutex_t mutex;
extern pthread_cond_t conditional;

// typedef struct {
//   int front, rear;
//   int size;
//   int** array;
// } Queue;

// Queue* createQueue();

// int isFull(Queue* queue);

// int isEmpty(Queue* queue);

// void enqueue(Queue* queue, int* client_socket);

// int * dequeue(Queue* queue);

void enqueue(int* item);
int* dequeue();
int* peek();


#endif
