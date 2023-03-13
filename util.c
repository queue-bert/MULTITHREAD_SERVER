#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "util.h"
#include "queue.h"
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/sendfile.h>

volatile sig_atomic_t flag = 0;

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
    char buffer[BUFSIZE+1];
    char filename[261];
    size_t num_bytes;
    struct stat st;
    int n;
    int msg_size = 0;
    char packet[1024];
    const char * mime_type;
    char req_method[10];
    char req_uri[256];
    char req_version[10] = "HTTP/?";
    char keep_conn[20];
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    int filefd;
    off_t offset = 0;
    char http_req[1000];
    char * crlf;


    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout , sizeof(timeout));
    for(;;)
    {
        memset(buffer, 0, BUFSIZE+1);
        memset(keep_conn, 0, 20);
        memset(packet, 0, 1024);
        msg_size = 0;
        offset = 0;
        buffer[BUFSIZE] = '\0';

        while(1)
        {
            if(check((n = read(client_socket, buffer+msg_size, sizeof(buffer)-msg_size-1)), "CONNECTION TIMEOUT"))
            {
                close(client_socket);
                return;
            }
            msg_size += n;
            if(msg_size > BUFSIZE - 1 || strstr(buffer, "\r\n\r\n")) break;
        }
        printf("BUFFER %s\n", buffer);

        

        if((crlf = strchr(buffer, '\r')) == NULL)
        {
            int p_sz = sprintf(packet,"%s 400 Bad Request\r\n\r\n", req_version);
            check(sendall(client_socket, packet, &p_sz), "Error writing 400 header to client\n");
            break;
        }
        else
        {
            int size = crlf - buffer;
            strncpy(http_req, buffer, size);
            http_req[size] = '\0';

            if ((n = sscanf(http_req, "%s %s %s", req_method, req_uri, req_version)) < 3)
            {
                int p_sz = sprintf(packet,"%s 400 Bad Request\r\n\r\n", req_version);
                check(sendall(client_socket, packet, &p_sz), "Error writing 400 header to client\n");
                break;
            }
        }
        
        if(strcmp(req_version, "HTTP/1.1") != 0 && strcmp(req_version, "HTTP/1.0") != 0)
        {
            int p_sz = sprintf(packet,"%s 505 HTTP Version Not Supported\r\n\r\n", req_version);
            check(sendall(client_socket, packet, &p_sz), "Error writing 505 header to client\n");
            break;
        }

        char * keep_alive = strstr(buffer, "Connection:");
        if(keep_alive != NULL)
        {
            // printf("ANOTHER REQUEST");
            n = sscanf(keep_alive+11, "%s", keep_conn);
            if(strcmp(keep_conn, "keep-alive") == 0)
                sprintf(keep_conn, "%s", "Keep-Alive");
            if(strcmp(keep_conn, "close") == 0)
                sprintf(keep_conn, "%s", "Close");
        }
        else
        {
            strcpy(keep_conn, "Close");
        }
            
        // printf("THIS IS CONNECTION TYPE: %s\n", keep_conn);

        if(strcmp(req_method, "GET") == 0)
        {
            if(valid_path(req_uri, filename))
            {
                // printf("NOT VALID");
                int p_sz = sprintf(packet,"%s 403 Forbidden\r\n\r\n", req_version);
                check(sendall(client_socket, packet, &p_sz), "Error writing 403 header to client\n");
                break;
            }

            if(check((filefd = open(filename, O_RDONLY)), "Could not open file"))
            {
                int p_sz = sprintf(packet,"%s 404 Not Found\r\n\r\n", req_version);
                check(sendall(client_socket, packet, &p_sz), "Error writing 404 header to client\n");
                break;
            }
            mime_type = get_mime_type(filename);
            num_bytes = get_fsize(filename, st);

            int p_sz = sprintf(packet,"%s 200 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\nConnection: %s\r\n\r\n", req_version, mime_type, (int)num_bytes, keep_conn);
            printf("PACKET: %s\n", packet);
            if(check(sendall(client_socket, packet, &p_sz), "Error writing 200 OKAY to client\n") < 0) break;

            while(num_bytes > 0)
            {
                ssize_t sent;
                if(check((sent = sendfile(client_socket, filefd, &offset, num_bytes)), "Error sending file")) break;
                num_bytes -= sent;
            }
            close(filefd);
        }
        else
        {
            int p_sz = sprintf(packet,"%s 405 Method Not Allowed\r\n\r\n", req_version);
            check(sendall(client_socket, packet, &p_sz), "Error writing 405 header to client\n");
            break;
        }

        if(strcmp(keep_conn, "Close") == 0)
            break;
    }
    close(client_socket);
}

void * thread_function()
{
    int *pclient;
    while(!flag)
    {
        pthread_mutex_lock(&mutex);
        while((!flag && (pclient = dequeue()) == NULL))
        {
            pthread_cond_wait(&conditional, &mutex);
        }
        pthread_mutex_unlock(&mutex);
        if(pclient != NULL)
        {
            connect_and_send(pclient);
            free(pclient);
        }
    }
    pthread_exit(NULL);
}

const char * get_mime_type(const char* file_path)
{
    const char* file_extension = strrchr(file_path, '.');
    if (file_extension) {
        if (strcmp(file_extension, ".html") == 0 || strcmp(file_extension, ".htm") == 0) {
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
    if (strcmp(req_uri, "/") == 0 || strcmp(req_uri, "/inside/") == 0)
    {
        sprintf(filename, "%s%s", DOCUMENT_ROOT, "/index.html");
    }
    else if (strstr(req_uri, "/inside/") != NULL)
    {
        sprintf(filename, "%s%s", DOCUMENT_ROOT, req_uri+7);
    }
    else
    {
        sprintf(filename, "%s%s", DOCUMENT_ROOT, req_uri);
    }

    if (access(filename, R_OK) == 0) {
        return 0;
    } else {

        return 1;
    }
    return 1;
}