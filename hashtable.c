#include "mysync.h"

// A C file that contains a powerful hashtable implementation that resizes itself when it gets too full


struct hashtable *create_hashtable(size_t size) {
    // A function that takes a size, and returns a hashtable with that size
    struct hashtable *hashtable = malloc_data(sizeof(struct hashtable));
    hashtable->size = size;
    hashtable->num_elements = 0;
    hashtable->table = calloc(size, sizeof(struct node *));
    if (hashtable->table == NULL) {
        fprintf(stderr, "Error: could not allocate memory for hashtable\n");
        exit(EXIT_FAILURE);
    }
    return hashtable;
}

int hash(char *key, int size) {
    // A function that takes a key and a size, and returns the hash of the key (tries to be as random as possible)
    int hash = 0;
    for (int i = 0; key[i] != '\0'; i++) {
        hash += key[i] * (i + 1);
    }
    return hash % size;
}

void free_node_data(void *data) {
    // A function that takes a file node, and frees the memory allocated for it
    int type = *(int *)data;
    if (type == 0) {
        struct dir_indexes *dir_indexes = (struct dir_indexes *)data;
        struct index *current_index = dir_indexes->head;  
        while (current_index != NULL) {
            struct index *next_index = current_index->next;
            free(current_index);
            current_index = next_index;
        }
    }
    free(data);
}

void resize(struct hashtable **hashtable, size_t size) {
    // A function that takes a hashtable, and resizes it to the given size
    struct hashtable *new_hashtable = create_hashtable(size);
    for (int i = 0; i < (*hashtable)->size; i++) {
        // Loop through the old hashtable
        struct node *current_node = (*hashtable)->table[i];
        while (current_node != NULL) {
            // Loop through the linked list
            put(&new_hashtable, current_node->name, current_node->data);
            free(current_node->name);
            free(current_node);
            current_node = current_node->next; 
        }
    }
    free((*hashtable)->table);
    free(*hashtable);
    *hashtable = new_hashtable;
}

void put(struct hashtable **hashtable, char *key, void *data) {
    // A function that takes a hashtable and a file node, and puts the file node into the hashtable
    int index = hash(key, (*hashtable)->size);
    // Check for collisions
    if ((*hashtable)->table[index] == NULL) {
        // If there are no collisions, put the data in the hashtable
        struct node *new_node = malloc_data(sizeof(struct node));
        new_node->name = strdup(key);
        new_node->data = data;
        new_node->next = NULL;
        (*hashtable)->table[index] = new_node;
        (*hashtable)->num_elements++;
        if ((*hashtable)->num_elements > 0.75*(*hashtable)->size) {
            // If the hashtable is too full, resize it
            resize(hashtable, (*hashtable)->size * 2);
        }
    } else {
        // If there is a collision, loop through the linked list until the end
        struct node *current_node = (*hashtable)->table[index];
        while (current_node != NULL) {
            if (strcmp(current_node->name, key) == 0) {
                // If the key already exists, replace the data
                free_node_data(current_node->data);
                current_node->data = data;
                return;
            }
            current_node = current_node->next;
        }
        // If the key does not exist, create a new node and put it at the end of the linked list
        struct node *new_node = malloc_data(sizeof(struct node));
        new_node->name = strdup(key);
        new_node->data = data;
        new_node->next = (*hashtable)->table[index];
        (*hashtable)->table[index] = new_node;
        (*hashtable)->num_elements++;
        if ((*hashtable)->num_elements > 0.75*(*hashtable)->size) {
            // If the hashtable is too full, resize it
            resize(hashtable, (*hashtable)->size * 2);
        }
    }
}

struct node *get(struct hashtable *hashtable, char *key) {
    // A function that takes a hashtable and a key, and returns the node with the key
    int index = hash(key, hashtable->size);
    // Check for collisions
    if (hashtable->table[index] == NULL) {
        // If there are no collisions, return NULL
        return NULL;
    } else {
        // If there is a collision, loop through the linked list until the end
        struct node *current_node = hashtable->table[index];
        while (current_node != NULL) {
            if (strcmp(current_node->name, key) == 0) {
                // If the key is found, return the data
                return current_node->data;
            }
            current_node = current_node->next;
        }
        // If the key is not found, return NULL
        return NULL;
    }
}



void delete(struct hashtable **hashtable, char *key) {
    // A function that takes a hashtable and a key, and deletes the node with the key
    int index = hash(key, (*hashtable)->size);
    // Check for collisions
    if ((*hashtable)->table[index] == NULL) {
        // If there are no collisions, return NULL
        return;
    } else {
        // If there is a collision, loop through the linked list until the end
        struct node *current_node = (*hashtable)->table[index];
        struct node *previous_node = NULL;
        while (current_node != NULL) {
            if (strcmp(current_node->name, key) == 0) {
                // If the key is found, delete the node
                free_node_data(current_node->data);
                free(current_node->name);
                if (previous_node == NULL) {
                    // If the node is the first node in the linked list, set the first node to the next node
                    (*hashtable)->table[index] = current_node->next;
                } else {
                    // If the node is not the first node in the linked list, set the previous node's next node to the next node
                    previous_node->next = current_node->next;
                }
                free(current_node);
                (*hashtable)->num_elements--;
                if ((*hashtable)->num_elements < 0.25*(*hashtable)->size && (*hashtable)->size > DEFAULT_HASHTABLE_SIZE) {
                    // If the hashtable is too empty, resize it
                    resize(hashtable, (*hashtable)->size / 2);
                }
                return;
            }
            previous_node = current_node;
            current_node = current_node->next;
        }
        // If the key is not found, return NULL
        return;
    }
}
