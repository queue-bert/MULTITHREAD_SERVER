#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "util.h"
#include "queue.h"
#include <string.h>
#include <pthread.h>

int sendall(int s, char *buf, int *len)
{
    int total = 0;
    int bytesleft = *len;
    int n;

    while(total < *len)
    {
        if((n = send(s, buf+total, bytesleft, 0)) <= 0)
        {
            break;
        }
        else
        {
            total += n;
            bytesleft -= n;
        }
    }

    *len = total;

    return n==-1?-1:0;
}

int get_fsize(char* file, struct stat st)
{
    if(stat(file, &st) == 0)
    {
        return (off_t)st.st_size;
    }
    else
    {
        perror("error determining the filesize");
        return 1;
    }
}

int check(int stat, char* message)
{
    if (stat == SOCKETERROR)
    {
        perror(message);
        return 1;
        //exit(1);
    }
    else
    {
        return 0;
    }
}

void connect_and_send(int * client_socket_fd)
{
    int client_socket = *client_socket_fd;
    char buffer[BUFSIZE];
    char filename[261];
    size_t num_bytes;
    struct stat st;
    int n;
    char packet[1024];
    char res_status[4];
    const char * mime_type;
    char req_method[10];
    char req_uri[256];
    char req_version[10];


    if(recv(client_socket, buffer, BUFSIZE, 0) <= 0)
    {
      perror("ERROR in recv()");
      return;
    }
    else
    {
        n = sscanf(buffer, "%s %s %s", req_method, req_uri, req_version);
        if (n < 3)
        {
            int p_sz = sprintf(packet,"%s 400 Bad Request\r\n\r\n", req_version);
            if(check(sendall(client_socket, packet, &p_sz), "Error writing header to client\n") < 0) return;
            return;
        }

        // if(strcmp(req_version, "HTTP/1.1") != 0 || strcmp(req_version, "HTTP/1.0") != 0)
        // {
        //     int p_sz = sprintf(packet,"%s 505 HTTP Version Not Supported\r\n\r\n", req_version);
        //     if(check(sendall(client_socket, packet, &p_sz), "Error writing header to client\n") < 0) return;
        //     return;
        // }
    }

    if(strcmp(req_method, "GET") == 0)
    {
        if(!valid_path(req_uri, filename))
        {
            int p_sz = sprintf(packet,"%s 403 Forbidden\r\n\r\n", req_version);
            if(check(sendall(client_socket, packet, &p_sz), "Error writing header to client\n") < 0) return;
            return;
        }
        
        FILE *fp = fopen(filename, "r");
        if (fp == NULL)
        {
            printf("Could not open file for reading");
            int p_sz = sprintf(packet,"%s 404 Not Found\r\n\r\n", req_version);
            if(check(sendall(client_socket, packet, &p_sz), "Error writing header to client\n") < 0) return;
            //return;
        }
        else
        {
            sprintf(res_status, "%s", "200");
            mime_type = get_mime_type(filename);
            num_bytes = get_fsize(filename, st);
        }

        int p_sz = sprintf(packet,"%s %s OK\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n", req_version, res_status , mime_type, (int)num_bytes);
        if(check(sendall(client_socket, packet, &p_sz), "Error writing header to client\n") < 0) return;

        while (num_bytes > 0)
        {
            memset(packet, 0, sizeof(packet));
            int chunk = BUFSIZE;
            if(num_bytes < BUFSIZE)
            {
                chunk = num_bytes;
            }
            int numb_read = (int) fread(packet, sizeof(char), chunk, fp);
            if(check(sendall(client_socket, packet, &numb_read), "Error writing file to client\n") < 0) return;
            num_bytes -= numb_read;
        }
    }
    else
    {
        int p_sz = sprintf(packet,"%s 405 Method Not Allowed\r\n\r\n", req_version);
        if(check(sendall(client_socket, packet, &p_sz), "Error writing header to client\n") < 0) return;
        return;
    }
    close(client_socket);
}

void * thread_function()
{
    int *pclient;
    for(;;)
    {
        pthread_mutex_lock(&mutex);
        if((pclient = dequeue()) == NULL)
        {
            pthread_cond_wait(&conditional, &mutex);
            pclient = dequeue();

        }
        pthread_mutex_unlock(&mutex);
        if(pclient != NULL)
        {
            connect_and_send(pclient);
        }
    }
}

const char* get_mime_type(const char* file_path) {
    const char* file_extension = strrchr(file_path, '.');
    if (file_extension) {
        if (strcmp(file_extension, ".html") == 0) {
            return "text/html";
        } else if (strcmp(file_extension, ".txt") == 0) {
            return "text/plain";
        } else if (strcmp(file_extension, ".png") == 0) {
            return "image/png";
        } else if (strcmp(file_extension, ".gif") == 0) {
            return "image/gif";
        } else if (strcmp(file_extension, ".jpg") == 0 || strcmp(file_extension, ".jpeg") == 0) {
            return "image/jpeg";
        } else if (strcmp(file_extension, ".ico") == 0) {
            return "image/x-icon";
        } else if (strcmp(file_extension, ".css") == 0) {
            return "text/css";
        } else if (strcmp(file_extension, ".js") == 0) {
            return "application/javascript";
        } else {
            return "application/octet-stream";
        }
    } else {
        return "application/octet-stream";
    }
}

int valid_path(char * req_uri, char * filename)
{

    if (strcmp(req_uri, "/") == 0)
    {
        sprintf(filename, "%s%s", DOCUMENT_ROOT, "/index.html");
        printf("%s\n", filename);
    }
    else
    {
        sprintf(filename, "%s%s", DOCUMENT_ROOT, req_uri);
        printf("%s\n", filename);
    }

    if (strstr(filename, "..") != NULL)
    {
        return 0;
    }

    return 1;
}

int rm_null(char * str, int n)
{
    int deletions = 0;
    char *p = str;
    char *q = str;

    // int len = strlen(str);
    // if (len > 0 && str[len-1] == '\0') {
    //     str[len-1] = ' ';
    // }

    
    while (*p)
    {
        if (*p == '\0' && deletions < n)
        {
            deletions++;
        }
        else
        {
            *q++ = *p;
        }
        p++;
    }
    
    *q = '\0';
    return deletions;
}
