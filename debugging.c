#include "mysync.h"

void print_all(struct hashtable *hashtable, struct file_names *all_names, char **directories) {
    // A function that takes a hashtable and a linked list of file names, and prints all the files in the hashtable
    struct file_names *current_name = all_names;
    printf("Master files:\n");
    while (current_name != NULL) {
        void *data = get(hashtable, current_name->name);
        int type = *(int *)data;
        if (type == 0) {
            printf("    %s (directory)\n", current_name->name);
        } else {
            struct file *file = (struct file *)data;
            printf("    %s (file) in directory %s\n", current_name->name, directories[file->directory_index]);
        }
        current_name = current_name->next;
    }
}
