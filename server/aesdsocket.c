/*
Autore: M.Ronchini 14 dicembre 2024

*/
#include "linkedlist.h"

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
// struct Node *headThread = NULL;
pthread_t threadtimer_id;
int server_socket = -1;
int client_socket = -1;
int file_fd = -1;
int tcounter = 0;
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

void cleanupsignal(int sign)
{

    unlink(FILENAME);
    syslog(LOG_INFO, "Removed file %s", FILENAME);
    running = 0;
    close(server_socket);

    syslog(LOG_INFO, "End process\n");
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

void *timerThread(void *value)
{
    char tsstring[2048];
    char outtext[1024];
    time_t t = 0;
    struct tm *tmp;
    syslog(LOG_INFO, "Starting timer thread ....");
    while (running)
    {
        usleep(TIMEFORTHREAD);
        memset(outtext, 0, 1024);
        memset(tsstring, 0, 2048);
        t = time(NULL);
        tmp = localtime(&t);
        strftime(outtext, sizeof(outtext), "%Y%m%d %H:%M:%S", tmp);
        sprintf(tsstring, "timestamp:%s\n", outtext);
        writefile(FILENAME, tsstring, strlen(tsstring));
    }
    syslog(LOG_INFO, "Stopping timer thread ....");
    pthread_exit(NULL);
}

void *socketThread(void *th_param)
{

    struct Node *myThread = (struct Node *)th_param;
    char *buffer = malloc(BUFFERSIZE);
    ssize_t bytes_received = 0;
    int bytesSent = 0;
    syslog(LOG_INFO, "Starting socket %d for thread %ld", myThread->client_socket, myThread->thread_id);
    if (!buffer)
    {
        syslog(LOG_INFO, "Error allocating buffer with malloc: %m");
        perror("Error allocating buffer with malloc");
        myThread->finished = 1;
        pthread_exit(NULL);
    }
    while (running)
    {

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
                bytesSent = send(client_socket, buffer, bytes_read, 0);
                if (bytesSent < 0)
                {
                    syslog(LOG_ERR, "Failed to send data to client: %m");
                    perror("Failed to send data to clien");
                }
                if (bytesSent > 0)
                {
                    syslog(LOG_ERR, "Sent %d bytes for thread %ld ", bytesSent, myThread->thread_id);
                    break;
                }
            }
        }
    }
    if (buffer)
        free(buffer);
    syslog(LOG_INFO, "Stopping socket %d for thread %ld", myThread->client_socket, myThread->thread_id);
    pthread_mutex_lock(&listMutex);
    myThread->finished = FINISHED;
    pthread_mutex_unlock(&listMutex);
    return th_param;
}

int serversocket()
{
    struct Node *myNode = NULL;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char client_ip[32];
    memset(client_ip, 0, 32);
    // int retv = 0;
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
    int true = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &true, sizeof(true));

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
        myNode = NULL;
        pthread_t thread_id;
        int retThread = 0;
        syslog(LOG_INFO, "Waiting for connection from socket  %d", server_socket);
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0)
        {
            syslog(LOG_ERR, "Failed to accept connection: %m");
            perror("Failed to accept connection");
            continue;
        }
        thread_id = 0;
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);
        myNode = createNode(thread_id, client_socket, 0);
        retThread = pthread_create(&myNode->thread_id, NULL, socketThread, myNode);
        if (retThread != 0)
        {
            perror("Thread creation failed");
            syslog(LOG_ERR, "Thread creation failed");
            close(client_socket);
            free(myNode);
            break;
        }
        appendNode(myNode);
        usleep(DELAY);
        removeFinishedThreads();
       
    }
    pthread_join(threadtimer_id, NULL);
    syslog(LOG_INFO, "Stopped timer thread %ld\n", threadtimer_id);

    close(server_socket);
    syslog(LOG_INFO, "Thread %d created. Bye ....", tcounter);
    close_log();
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
    signal_handler();

    if (!demone)
    {

        ret = serversocket();
    }
    else
    {

        pid_t pid = fork();

        if (pid == -1)
        {
            perror("Error in fork");
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

    return ret;
}