#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"
#include <netinet/in.h>
#include <pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t conditional = PTHREAD_COND_INITIALIZER;


int* queue[QUEUE_SIZE];
int queue_counter = 0;

void enqueue(int* item) {
  if (queue_counter == QUEUE_SIZE) {
    return;
    // printf("Queue is full\n");
  } else {
    queue[queue_counter] = item;
    queue_counter++;
  }
}

int* dequeue() {
  if (queue_counter == 0) {
    printf("Queue is empty\n");
    return NULL;
  } else {
    int* item = queue[0];
    for (int i = 0; i < queue_counter - 1; i++) {
      queue[i] = queue[i + 1];
    }
    queue_counter--;
    return item;
  }
}

int* peek() {
  if (queue_counter == 0) {
    printf("Queue is empty\n");
    return NULL;
  } else {
    return queue[0];
  }
}
// Queue * createQueue() {
//   Queue* queue = (Queue*)malloc(sizeof(Queue));
//   queue->front = queue->size = 0;
//   queue->rear = QUEUE_SIZE - 1;
//   queue->array = (int**)malloc(QUEUE_SIZE * sizeof(int*));
//   return queue;
// }

// int isFull(Queue* queue) {
//   return (queue->size == QUEUE_SIZE);
// }

// int isEmpty(Queue* queue) {
//   return (queue->size == 0);
// }

// void enqueue(Queue* queue, int * client_socket) {
//   // pthread_mutex_lock(&mutex);
//   if (isFull(queue))
//     return;
//   queue->rear = (queue->rear + 1) % QUEUE_SIZE;
//   queue->array[queue->rear] = client_socket;
//   queue->size = queue->size + 1;
//   // pthread_cond_signal(&conditional);
//   // pthread_mutex_unlock(&mutex);
// }

// int * dequeue(Queue* queue) {
//   // pthread_mutex_lock(&mutex);
//   if (isEmpty(queue))
//     return NULL;
//     // pthread_cond_wait(&conditional, &mutex);
//   int * client_socket = queue->array[queue->front];
//   queue->front = (queue->front + 1) % QUEUE_SIZE;
//   queue->size = queue->size - 1;
//   // pthread_mutex_unlock(&mutex);
//   return client_socket;
// }