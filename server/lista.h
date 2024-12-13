#include <stdio.h>
#include <stdlib.h>

// Nodo della lista
struct Node {
    pthread_t thread_id;  // Thread identifier
    int client_socket;    // Client socket descriptor
    int finished;
    struct Node *next;  // Puntatore al nodo successivo
};

// Funzione per creare un nuovo nodo
struct Node* create_node(pthread_t thread_id, int client_socket, int finished) {
    struct Node *new_node = (struct Node *)malloc(sizeof(struct Node));
    if (!new_node) {
        perror("Errore nell'allocazione della memoria");
        exit(EXIT_FAILURE);
    }
    new_node->thread_id = thread_id; // Assegna il valore
    new_node->client_socket = client_socket;
    new_node->finished = finished;
    new_node->next = NULL;  // Il nuovo nodo punta a NULL
    return new_node;
}

// Funzione per aggiungere un nodo alla fine della lista
/*void append_node(struct Node **head, struct Node **new_node) {
  
    if (*head == NULL) {
        // Se la lista è vuota, il nuovo nodo diventa la testa
        *head = new_node;
    } else {
        // Cerca l'ultimo nodo
        struct Node *current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node; // Collega l'ultimo nodo al nuovo nodo
    }
}
*/
void append_node(struct Node *head, struct Node *new_node) {
  
    if (head == NULL) {
        // Se la lista è vuota, il nuovo nodo diventa la testa
        head = new_node;
    } else {
        // Cerca l'ultimo nodo
        struct Node *current = head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node; // Collega l'ultimo nodo al nuovo nodo
    }
}
// Funzione per stampare tutti i nodi della lista
/*void print_list(struct Node *head) {
    struct Node *current = head;
    printf("Elementi nella lista: ");
    while (current != NULL) {
        printf("%d ", current->thread_id);
        current = current->next;
    }
    printf("\n");
}*/

// Funzione per rimuovere un nodo con un dato valore
/*void remove_node(struct Node **head, struct Node *mynode) {
    struct Node *current = *head;
    struct Node *previous = NULL;

    while (current != NULL && current->thread_id != mynode->thread_id && current->client_socket != mynode->client_socket) {
        previous = current;
        current = current->next;
    }

    if (current == NULL) {
       // printf("Valore %d non trovato nella lista.\n", thread_id);
        return;
    }

    // Se il nodo da rimuovere è la testa
    if (previous == NULL) {
        *head = current->next;
    } else {
        previous->next = current->next;
    }
  
    free(current); // Libera la memoria del nodo
}*/
void remove_node(struct Node *head, struct Node *mynode) {
    struct Node *current = head;
    struct Node *previous = NULL;

    while (current != NULL && current->thread_id != mynode->thread_id && current->client_socket != mynode->client_socket) {
        previous = current;
        current = current->next;
    }

    if (current == NULL) {
       // printf("Valore %d non trovato nella lista.\n", thread_id);
        return;
    }

    // Se il nodo da rimuovere è la testa
    if (previous == NULL) {
        head = current->next;
    } else {
        previous->next = current->next;
    }
  
    free(current); // Libera la memoria del nodo
}


// Funzione per liberare tutta la lista
void free_list(struct Node *head) {
    struct Node *current = head;
    struct Node *next_node;

    while (current != NULL) {
        next_node = current->next;
        free(current);
        current = next_node;
    }
    head = NULL; // Imposta la testa a NULL
    //printf("Lista liberata.\n");
}
