#include "mysync.h"

void print_all(Hashtable *hashtable, Relpaths *relpaths, char **directories) {
    // A function that takes a hashtable and a linked list of file or directory names, and prints them all in the hashtable
    Relpaths *current_name = relpaths; // Loop through the linked list
    while (current_name != NULL) {
        void *data = get(hashtable, current_name->relpath); // Get the data from the hashtable
        int type = *(int *)data; // Get the type of the data
        if (type == 0) {
            Dir_indexes *dir_indexes = (Dir_indexes *)data; // If the type is 0, it is a directory
            printf("    \"%s\" which is %s\n", current_name->relpath, dir_indexes->valid ? "wanted" : "not wanted"); // Print the directory and whether it is wanted or not
        } else {
            File *file = (File *)data; // If the type is 1, it is a file
            printf("    \"%s\" in directory %s\n", current_name->relpath, directories[file->directory_index]); // Print the file and the directory it is in
        }
        current_name = current_name->next; // Go to the next name
    }
}
