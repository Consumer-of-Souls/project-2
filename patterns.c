#include "mysync.h"

void enqueue_pattern(struct pattern **head, char *glob) {
    // A function that takes a glob pattern and converts it to a regex pattern, and then compiles it and adds it to the linked list
    char *regex = glob2regex(glob);
    if (regex == NULL) {
        fprintf(stderr, "Error: could not convert glob %s to regex\n", glob);
        exit(EXIT_FAILURE);
    }
    struct pattern *new_pattern = malloc_data(sizeof(struct pattern));
    int err = regcomp(&(new_pattern->regex), regex, REG_EXTENDED | REG_NOSUB);
    if (err != 0) {
        fprintf(stderr, "Error: could not compile regex %s\n", regex);
        exit(EXIT_FAILURE);
    }
    free(regex);
    new_pattern->next = *head;
    *head = new_pattern;
}

int check_patterns(struct pattern *head, char *filename) {
    // A function that takes a linked list of patterns and a filename, and returns 1 if the filename matches any of the patterns, and 0 otherwise
    struct pattern *current_pattern = head;
    while (current_pattern != NULL) {
        int err = regexec(&(current_pattern->regex), filename, 0, NULL, 0);
        if (err == 0) {
            return 1;
        } else if (err != REG_NOMATCH) {
            fprintf(stderr, "Error: could not execute regex\n");
            exit(EXIT_FAILURE);
        }
        current_pattern = current_pattern->next;
    }
    return 0;
}
