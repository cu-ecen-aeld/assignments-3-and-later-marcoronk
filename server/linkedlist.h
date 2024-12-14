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


struct Node {
    pthread_t thread_id;  // Thread identifier
    int client_socket;    // Client socket descriptor
    int finished;
    struct Node *next;  // Puntatore al nodo successivo
};

struct Node* head = NULL;
pthread_mutex_t listMutex;

// Funzione per creare un nuovo nodo
struct Node* createNode(pthread_t thread_id, int client_socket, int finished) {
    pthread_mutex_lock(&listMutex);
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
    if (!newNode) {
        printf("Errore nell'allocazione della memoria.\n");
        exit(1);
    }
    newNode->thread_id = thread_id;
    newNode->client_socket = client_socket;
    newNode->finished = finished;
    newNode->next = NULL;
    pthread_mutex_unlock(&listMutex);
    return newNode;
}



void appendNode(struct Node* newNode) {
    pthread_mutex_lock(&listMutex); // Blocco del mutex
    
    if (head == NULL) {
        head = newNode;
    } else {
        struct Node* temp = head;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = newNode;
    }

    pthread_mutex_unlock(&listMutex); // Sblocco del mutex
}

void freeList() {
    pthread_mutex_lock(&listMutex); // Blocco del mutex

    struct Node* temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        free(temp);
    }

    pthread_mutex_unlock(&listMutex); // Sblocco del mutex
}

void removeFinishedThreads() {
    pthread_mutex_lock(&listMutex); 
    struct Node *current = NULL;
    struct Node *temp = NULL;
    while (current != NULL)
        {           
            if (current->finished == 1)            
            {               
                
                syslog(LOG_INFO, "-> Deleting thread for socket %d", current->client_socket);
                pthread_join(current->thread_id, NULL);
                close(current->client_socket);                
                temp = current;
                current = current->next;                                
                free(temp);
            }
            else 
                current = current->next;          
    }
    pthread_mutex_unlock(&listMutex); 
}
