#include "mysync.h"

void free_patterns(struct pattern *pattern) {
    // A function that takes a linked list of patterns and frees all the memory allocated for it
    struct pattern *current_pattern = pattern; // Set the current pattern to the head of the linked list
    struct pattern *temp = NULL; // Initialize a temporary pattern for storing the next pattern
    while (current_pattern != NULL) {
        // Loop through the linked list
        temp = current_pattern->next; // Set the temporary pattern to the next pattern
        regfree(&(current_pattern->regex)); // Free the memory allocated for the regex
        free(current_pattern); // Free the memory allocated for the current pattern
        current_pattern = temp; // Set the current pattern to the temporary pattern
    }
}

void enqueue_pattern(struct pattern **head, char *glob) {
    // A function that takes a linked list of patterns and a glob, and adds the glob to the linked list as a regex
    char *regex = glob2regex(glob); // Convert the glob to a regex
    if (regex == NULL) {
        // If the glob could not be converted to a regex, print an error message and exit the program
        fprintf(stderr, "Error: could not convert glob %s to regex\n", glob);
        exit(EXIT_FAILURE);
    }
    struct pattern *new_pattern = malloc_data(sizeof(struct pattern)); // Allocate memory for the new pattern
    int err = regcomp(&(new_pattern->regex), regex, REG_EXTENDED | REG_NOSUB); // Compile the regex and add it to the new pattern
    if (err != 0) {
        // If the regex could not be compiled, print an error message and exit the program
        fprintf(stderr, "Error: could not compile regex %s\n", regex);
        free(regex);
        exit(EXIT_FAILURE);
    }
    free(regex); // Free the memory allocated for the regex
    new_pattern->next = *head; // Set the next pattern to the head of the linked list
    *head = new_pattern; // Set the head of the linked list to the new pattern
}

bool check_patterns(struct pattern *head, char *filename) {
    // A function that takes a linked list of patterns and a filename, and returns true if the filename matches any of the patterns, and false otherwise
    struct pattern *current_pattern = head; // Set the current pattern to the head of the linked list
    while (current_pattern != NULL) {
        // Loop through the linked list
        int err = regexec(&(current_pattern->regex), filename, 0, NULL, 0); // Execute the regex
        if (err == 0) {
            // If the regex matches the filename, return true
            return true;
        } else if (err != REG_NOMATCH) {
            // If the regex could not be executed, print an error message and exit the program
            fprintf(stderr, "Error: could not execute regex\n");
            exit(EXIT_FAILURE);
        }
        current_pattern = current_pattern->next; // Set the current pattern to the next pattern
    }
    return false; // Return false if the filename does not match any of the patterns
}
