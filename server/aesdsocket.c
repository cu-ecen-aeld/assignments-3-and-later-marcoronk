/*
Autore: M.Ronchini 7 dicembre 2024

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>
#include <syslog.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/queue.h>
#include "lista.h"

#define OK 0
#define KO -1
#define ERR 1
#define FINISHED 1
#define PORT 9000
#define BUFFERSIZE 48000
#define DELAY 10000
#define TIMEFORTHREAD 10000000
#define FILENAME "/var/tmp/aesdsocketdata"
pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
struct Node *headThread = NULL;
pthread_t threadtimer_id;
int server_socket = -1;
int client_socket = -1;
int file_fd = -1;
int runThreadTimer=1;
static volatile int running = 1;
int counter = 0;
int ret = 0;



void open_log()
{
    openlog("aesdsocket", LOG_PID | LOG_CONS, LOG_USER);
}

void close_log()
{
    closelog();
}


void cleanupsignal()
{
    struct Node *current = headThread;
    syslog(LOG_INFO, "Cleanup aesdocket started\n");
    runThreadTimer=0;  
    pthread_join(threadtimer_id,NULL);
    syslog(LOG_INFO, "Cleanup stopped thimer thread\n");
    running = 0;
    while (current != NULL)
    {
        syslog(LOG_INFO, "Cleanup closing Socket %d for thread %s", current->client_socket, current->thread_id );
        close(current->client_socket);
        current->finished=FINISHED;
        pthread_join(current->thread_id,NULL);
        //remove_node(headThread, current);                    
        current = current->next;
        syslog(LOG_INFO, "Cleanup closed Socket %d for thread %s", current->client_socket, current->thread_id );
    }
    free_list(headThread);
    unlink(FILENAME);
    syslog(LOG_INFO, "Removed file %s", FILENAME);
    close_log();
    exit(0);
}

void signal_handler()
{
    struct sigaction sa;
    sa.sa_handler = cleanupsignal;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

int readfile(char *path, char *buffer, int size)
{
    pthread_mutex_lock(&list_mutex);
    FILE *file = fopen(path, "r");
    int read_bytes = 0;
    if (file == NULL)
    {
        syslog(LOG_ERR, "Error reading in file %s:\n", path);
        return KO;
    }
    if (buffer)
    {
        read_bytes = fread(buffer, sizeof(char), size, file);
    }
    fclose(file);
    pthread_mutex_unlock(&list_mutex);

    syslog(LOG_INFO, "Read %s %d:\n", buffer, read_bytes);

    return read_bytes;
}

int writefile(char *path, char *buffer, int size)
{

    pthread_mutex_lock(&list_mutex);
    FILE *file = fopen(path, "a");
    int written_bytes = 0;
    if (file == NULL)
    {
        syslog(LOG_ERR, "Error writing in file %s:\n", path);
        return KO;
    }
    if (buffer)
    {
        written_bytes = fwrite(buffer, sizeof(char), size, file);
    }
    fclose(file);
    pthread_mutex_unlock(&list_mutex);
    syslog(LOG_INFO, "Write %s %d %d:\n", buffer, size, written_bytes);

    return written_bytes;
}

void* timerThread(int value)
{
    char tsstring[2048];
    char outtext[1024];    
    time_t t = 0;  
    struct tm *tmp;
    syslog(LOG_INFO, "Starting timer thread ....");
    while (running)
    {
        if (runThreadTimer == 0)
         break;
        usleep(TIMEFORTHREAD);
        memset(outtext, 0, 1024);
        memset(tsstring, 0, 2048);
        t = time(NULL);
        tmp = localtime(&t);
        strftime(outtext, sizeof(outtext), "%Y%m%d %H:%M:%S", tmp);
        sprintf(tsstring,"timestamp:%s\n",outtext);
        writefile(FILENAME, tsstring, strlen(tsstring));
    }
    syslog(LOG_INFO, "Stopping timer thread ....");
    pthread_exit(NULL);
}

void* socketThread(void *th_param)
{

    struct Node* myThread = (struct Node*) th_param;
    char *buffer = malloc(BUFFERSIZE);    
    ssize_t bytes_received = 0;
    syslog(LOG_INFO, "Starting socket %d for thread %ld",myThread->client_socket, myThread->thread_id);
    if (!buffer)
    {
        syslog(LOG_INFO, "Error allocating buffer with malloc: %m");
        perror("Error allocating buffer with malloc");
        myThread->finished = 1;
        pthread_exit(NULL);
    }
    while (running)
    {
        if (myThread->finished == FINISHED)
          break;
        memset(buffer, 0, BUFFERSIZE);
        bytes_received = recv(myThread->client_socket, buffer, BUFFERSIZE - 1, 0);
        if (bytes_received == -1)
        {
            syslog(LOG_ERR, "Error reading socket recv: %m");
            perror("Error reading socket recv");
            break;
        }
        if (bytes_received == 0)
        {
            syslog(LOG_INFO, "No more byte on socket: %m");
            break;
        }
        else
        {
            syslog(LOG_INFO, "Received  %ld", bytes_received);
            writefile(FILENAME, buffer, bytes_received);
            syslog(LOG_INFO, "Written on file %ld bytes", bytes_received);
            if (strchr(buffer, '\n'))
            {
                syslog(LOG_INFO, "Found CR in %s", buffer);
                memset(buffer, 0, BUFFERSIZE);
                ssize_t bytes_read = readfile(FILENAME, buffer, BUFFERSIZE);
                ret = send(client_socket, buffer, bytes_read, 0);
                if (ret < 0)
                {
                    syslog(LOG_ERR, "Failed to send data to client: %m");
                    perror("send");
                    break;
                }
            }
        }
    }
    syslog(LOG_INFO, "Stopping socket %d for thread %ld",myThread->client_socket, myThread->thread_id);
    close(myThread->client_socket);
    myThread->finished = FINISHED;
    pthread_exit(NULL);
}

int serversocket()
{

    struct Node *myNode = NULL;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    //ssize_t bytes_received = 0;
    char client_ip[32];
    memset(client_ip, 0, 32);
    int stopciclo = 1;
    syslog(LOG_INFO, "Starting aessocket....");
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        syslog(LOG_ERR, "Failed to create server socket: %m");
        perror("socket");
        return KO;
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        syslog(LOG_ERR, "Failed to bind server socket: %m");
        perror("Failed to bind server socket");
        return KO;
    }

    if (listen(server_socket, 5) < 0)
    {
        syslog(LOG_ERR, "Failed to listen on server socket: %m");
        perror("Failed to listen on server socket");
        return KO;
    }
   

   pthread_create(&threadtimer_id, NULL, timerThread, 0);


    while (running)
    {
        pthread_t thread_id=0;
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0)
        {
            syslog(LOG_ERR, "Failed to accept connection: %m");
            perror("Failed to accept connection");
            return KO;
        }

        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);
        struct Node *myNode = create_node(thread_id, client_socket, 0);
        if (pthread_create(&myNode->thread_id, NULL, socketThread, myNode) != 0)
        {
            perror("Thread creation failed");
            syslog(LOG_ERR, "Thread creation failed");
            close(client_socket);
            break;
        }
        append_node(headThread, myNode);
        usleep(DELAY);
        struct Node *current = headThread;
        while (current != NULL)
        {
            if (current->finished == FINISHED)
            {
                pthread_join(current->thread_id, NULL);
                close(current->client_socket);
                remove_node(headThread, current);
                syslog(LOG_INFO, "Closed connection from %s", client_ip);
                syslog(LOG_INFO, "Closed Socket %d for thread %s", current->client_socket, current->thread_id );
            }
            current = current->next;
        }
        if (headThread == NULL)
           break;
    }

    //cleanup();
    runThreadTimer=0;
    cleanupsignal();

    return 0;
}

int main(int argc, char **argv)
{
    int demone = 0;
    int resp = 0;
    open_log();

    while ((resp = getopt(argc, argv, "d")) != -1)
    {
        switch (resp)
        {
        case 'd':
            demone = 1;
            break;

        default:
            demone = 0;
        }
    }
    // int ret = 0;
    signal_handler();

    if (!demone)
    {
       // while (1)
       // {
            ret = serversocket();
       // }
    }
    else
    {

        pid_t pid = fork();

        if (pid == -1)
        {
            perror("Error in form");
            exit(1);
        }
        else if (pid == 0)
        {           
            ret = serversocket();
        }
        else
        {

            ret = 0;
        }
    }

    return 0;
}