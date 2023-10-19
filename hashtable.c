#include "mysync.h"

// A C file that contains a powerful hashtable implementation that resizes itself when it gets too full

Hashtable *create_hashtable(size_t size) {
    // A function that takes a size, and returns a hashtable with that size
    Hashtable *hashtable = malloc_data(sizeof(Hashtable)); // Allocate memory for the hashtable
    hashtable->size = size; // Set the size of the hashtable
    hashtable->num_elements = 0; // Initialize the number of elements in the hashtable to 0
    hashtable->table = calloc(size, sizeof(Node *)); // Allocate memory for the table of file nodes (uses calloc so all pointers are initialized to NULL)
    if (hashtable->table == NULL) {
        // If calloc fails, print an error message and exit the program
        fprintf(stderr, "Error: could not allocate memory for hashtable\n");
        free(hashtable);
        exit(EXIT_FAILURE);
    }
    return hashtable; // Return the hashtable
}

unsigned int hash(char *key, int size) {
    // A function that takes a key and a size, and returns the hash of the key (tries to be as random as possible)
    unsigned int hash = 5381; // Initialize the hash to 5381 (the sexiest prime number)
    for (int i = 0; i < strlen(key); i++) {
        // Loop through the key
        hash = ((hash << 5) + hash) + key[i]; // Calculate the hash
    }
    return hash % size; // Return the hash modulo the size so it is within the range of the table
}

void free_node_data(void *data) {
    // A function that takes file node data and frees the memory allocated for it
    int type = *(int *)data; // cast the data to an int to get the type
    if (type == 0) {
        // If the type is 0, the data is a dir_indexes struct and more needs to be done to free the memory
        Dir_indexes *dir_indexes = (Dir_indexes *)data; // Cast the data to a dir_indexes struct
        Index *current_index = dir_indexes->head; // Loop through the indexes
        while (current_index != NULL) {
            Index *next_index = current_index->next; // Store the next index
            free(current_index); // Free the current index
            current_index = next_index; // Set the current index to the next index
        }
    }
    free(data); // Free the memory allocated for the data
}

void resize(Hashtable **hashtable, size_t size) {
    // A function that takes a hashtable, and resizes it to the given size
    Hashtable *new_hashtable = create_hashtable(size); // Create a new hashtable with the given size
    for (int i = 0; i < (*hashtable)->size; i++) {
        // Loop through the old hashtable
        Node *current_node = (*hashtable)->table[i]; // Get the current node
        while (current_node != NULL) {
            // Loop through the linked list in the current node
            put(&new_hashtable, current_node->name, current_node->data); // Put the node into the new hashtable
            free(current_node->name); // Free the memory allocated for the old node's name
            free(current_node); // Free the memory allocated for the old node
            current_node = current_node->next; // Set the current node to the next node
        }
    }
    // Free the memory allocated for the old hashtable and set the old hashtable to the new hashtable
    free((*hashtable)->table);
    free(*hashtable);
    *hashtable = new_hashtable;
}

void put(Hashtable **hashtable, char *key, void *data) {
    // A function that takes a hashtable, a key, and data, and puts the data into the hashtable with the key
    unsigned int index = hash(key, (*hashtable)->size); // Get the index of the key
    // Check for collisions
    if ((*hashtable)->table[index] == NULL) {
        // If there are no collisions, put the data in the hashtable
        Node *new_node = malloc_data(sizeof(Node)); // Allocate memory for the new node
        new_node->name = strdup(key); // Copy the key into the new node
        new_node->data = data; // Set the data of the new node to the data
        new_node->next = NULL; // Set the next node to NULL
        (*hashtable)->table[index] = new_node; // Set the node in the hashtable to the new node
    } else {
        // If there is a collision, loop through the linked list until the end
        Node *current_node = (*hashtable)->table[index]; // Get the first node in the linked list
        while (current_node != NULL) {
            // Loop through the linked list
            if (strcmp(current_node->name, key) == 0) {
                // If the key already exists, replace the data
                free_node_data(current_node->data); // Free the memory allocated for the old data
                current_node->data = data; // Set the data to the new data
                return;
            }
            current_node = current_node->next; // Set the current node to the next node
        }
        // If the key doesn't exist, add the node to the beginning of the linked list
        Node *new_node = malloc_data(sizeof(Node)); // Allocate memory for the new node
        new_node->name = strdup(key); // Copy the key into the new node
        new_node->data = data; // Set the data of the new node to the data
        new_node->next = (*hashtable)->table[index]; // Set the next node to the first node in the linked list
        (*hashtable)->table[index] = new_node; // Set the node in the hashtable to the new node
    }
    (*hashtable)->num_elements++; // Increment the number of elements in the hashtable
    if ((*hashtable)->num_elements > 0.75*(*hashtable)->size) {
        // If the hashtable is too full, resize it (double the size)
        resize(hashtable, (*hashtable)->size * 2);
    }
}

void *get(Hashtable *hashtable, char *key) {
    // A function that takes a hashtable and a key, and returns the data with the key
    unsigned int index = hash(key, hashtable->size); // Get the index of the key
    // Check for collisions
    if (hashtable->table[index] == NULL) {
        // If there are no collisions, return NULL
        return NULL;
    }
    // If there is a collision, loop through the linked list until the end
    Node *current_node = hashtable->table[index]; // Get the first node in the linked list
    while (current_node != NULL) {
        // Loop through the linked list
        if (strcmp(current_node->name, key) == 0) {
            // If the key is found, return the data
            return current_node->data;
        }
        current_node = current_node->next; // Set the current node to the next node
    }
    // If the key is not found, return NULL
    return NULL;
}



void delete(struct hashtable **hashtable, char *key) {
    // A function that takes a hashtable and a key, and deletes the node with the key
    unsigned int index = hash(key, (*hashtable)->size); // Get the index of the key
    // Check for collisions
    if ((*hashtable)->table[index] == NULL) {
        // If there are no collisions, return
        return;
    }
    // If there is a collision, loop through the linked list until the end
    Node *current_node = (*hashtable)->table[index]; // Get the first node in the linked list
    Node *previous_node = NULL; // Initialize the previous node to NULL
    while (current_node != NULL) {
        // Loop through the linked list
        if (strcmp(current_node->name, key) == 0) {
            // If the key is found, delete the node
            free_node_data(current_node->data); // Free the memory allocated for the data
            free(current_node->name); // Free the memory allocated for the name
            if (previous_node == NULL) {
                // If the node is the first node in the linked list, set the first node to the next node
                (*hashtable)->table[index] = current_node->next;
            } else {
                // If the node is not the first node in the linked list, set the previous node's next node to the next node
                previous_node->next = current_node->next;
            }
            free(current_node); // Free the memory allocated for the node
            (*hashtable)->num_elements--; // Decrement the number of elements in the hashtable
            if ((*hashtable)->num_elements < 0.25*(*hashtable)->size && (*hashtable)->size > DEFAULT_HASHTABLE_SIZE) {
                // If the hashtable is too empty, resize it (half the size)
                resize(hashtable, (*hashtable)->size / 2);
            }
            return;
        }
        previous_node = current_node; // Set the previous node to the current node
        current_node = current_node->next; // Set the current node to the next node
    }
    // If the key is not found, return
    return;
}
