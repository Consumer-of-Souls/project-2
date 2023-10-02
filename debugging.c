#include "mysync.h"

void print_directories(struct file_node **directory_contents, char **directories, int num_directories) {
    for (int i = 0; i < num_directories; i++) {
        printf("Directory %s:\n", directories[i]);
        struct file_node *current_node = directory_contents[i];
        while (current_node != NULL) {
            struct file *current_file = current_node->file;
            printf("    %s\n", current_file->name);
            current_node = current_node->next;
        }
    }
}
