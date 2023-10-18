#include "mysync.h"

void *malloc_data(size_t size) {
    // Allocates memory for data and checks if malloc fails
    void *new_data = malloc(size); // Allocate memory for the new data
    if (new_data == NULL) {
        // If malloc fails, print an error message and exit the program
        fprintf(stderr, "Error: Failed to allocate memory for new data\n");
        exit(EXIT_FAILURE);
    }
    return new_data; // Return the new data
}
