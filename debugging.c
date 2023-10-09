#include "mysync.h"

void print_all(struct hashtable *hashtable, struct relpaths *relpaths, char **directories) {
    // A function that takes a hashtable and a linked list of file names, and prints all the files in the hashtable
    struct relpaths *current_name = relpaths;
    while (current_name != NULL) {
        void *data = get(hashtable, current_name->relpath);
        int type = *(int *)data;
        if (type == 0) {
            // prints whether a directory is valid or invalid
            printf("    \"%s\" which is %s\n", current_name->relpath, ((struct dir_indexes *)data)->valid ? "wanted" : "not wanted");
        } else {
            struct file *file = (struct file *)data;
            printf("    \"%s\" in directory %s\n", current_name->relpath, directories[file->directory_index]);
        }
        current_name = current_name->next;
    }
}
