#include "mysync.h"

void free_file(struct file *file) {
    // Frees the memory allocated for the file
    free(file->name);
    free(file->type);
    free(file);
}

void free_patterns(struct pattern *pattern) {
    // Frees the memory allocated for the patterns
    struct pattern *current_pattern = pattern;
    while (current_pattern != NULL) {
        struct pattern *next_pattern = current_pattern->next;
        regfree(&(current_pattern->regex));
        free(current_pattern);
        current_pattern = next_pattern;
    }
}

void free_flags(struct flags *flags) {
    // Frees the memory allocated for the flags
    free_patterns(flags->ignore1);
    free_patterns(flags->only1);
    free(flags);
}

void *malloc_data(size_t size) {
    // Allocates memory for data and checks if malloc fails
    void *new_data = malloc(size); // Allocate memory for the new data
    if (new_data == NULL) {
        // If malloc fails, print an error message and exit the program
        fprintf(stderr, "Error: Failed to allocate memory for new data\n");
        exit(EXIT_FAILURE);
    }
    return new_data;
}
