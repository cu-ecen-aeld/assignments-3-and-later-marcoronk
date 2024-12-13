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

#define OK 0
#define KO -1
#define ERR 1
#define PORT 9000
#define BUFFERSIZE 48000

#define FILENAME "/var/tmp/aesdsocketdata"

int server_socket = -1;
int client_socket = -1;
int file_fd = -1;
char *buffer = NULL;
int counter = 0;
int ret = 0;

FILE *fileread;
FILE *filewrite;

void open_log()
{
    openlog("aesdsocket", LOG_PID | LOG_CONS, LOG_USER);
}

void close_log()
{
    closelog();
}

void cleanup()
{
    if (server_socket >= 0)
        close(server_socket);
    if (client_socket >= 0)
        close(client_socket);
    if (fileread)
        fclose(fileread);
    if (filewrite)
        fclose(filewrite);
}

void cleanupsignal()
{
    syslog(LOG_INFO, "Cleanup start");
    cleanup();

    if (buffer)
        free(buffer);
    unlink(FILENAME);
    close_log();
    exit(1);
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
        fclose(file);
    }
    syslog(LOG_INFO, "Read %s %d:\n", buffer, read_bytes);

    return read_bytes;
}

int writefile(char *path, char *buffer, int size)
{

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
        fclose(file);
    }
    syslog(LOG_INFO, "Write %s %d %d:\n", buffer, size, written_bytes);

    return written_bytes;
}

int serversocket()
{

    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    ssize_t bytes_received = 0;
    // ssize_t bytes_written = 0;
    char client_ip[32];
    memset(client_ip, 0, 32);    
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

    client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_socket < 0)
    {
        syslog(LOG_ERR, "Failed to accept connection: %m");
        perror("Failed to accept connection");
        return KO;
    }

    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
    syslog(LOG_INFO, "Accepted connection from %s", client_ip);
    buffer = malloc(BUFFERSIZE);
    if (!buffer)
    {
        syslog(LOG_ERR, "Error allocating buffer with malloc: %m");
        perror("Error allocating buffer with malloc");
        close(client_socket);
        return KO;
    }

    while (1)
    {
        memset(buffer, 0, BUFFERSIZE);
        bytes_received = recv(client_socket, buffer, BUFFERSIZE - 1, 0);
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

    cleanup();
    syslog(LOG_INFO, "Closed connection from %s", client_ip);

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
        while (1)
        {
            ret = serversocket();
        }
    } else {

         pid_t pid = fork();
        
        if (pid == -1)
        {
            perror("Error in form");
            exit(1);
        }
        else if (pid == 0)
        {
            // Child logic
            while (1)
            {
                ret = serversocket();
            }
        }
        else
        {
            
            ret = 0;
        }


    }

    
    return 0;
}