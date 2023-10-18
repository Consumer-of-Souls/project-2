#include "mysync.h"

void create_directory(char *relpath, char *parent_dir, struct flags *flags) {
    // A function that takes a relative path, a parent directory, and a flags struct, and creates the directory in the parent directory
    char *dirpath = malloc_data(strlen(parent_dir) + strlen(relpath) + 2); // Allocate memory for the directory path
    sprintf(dirpath, "%s/%s", parent_dir, relpath); // Create the directory path by concatenating the parent directory and the relative path
    if (!flags->no_sync_flag) {
        // If the -n flag was not passed, create the directory
        int result = mkdir(dirpath, 0777);
        if (result == -1) {
            // If mkdir fails, print an error message and exit the program
            fprintf(stderr, "Error: could not create directory %s\n", dirpath);
            free(dirpath);
            exit(EXIT_FAILURE);
        }
    }
    VERBOSE_PRINT("Created directory %s as it did not exist\n", dirpath);
    free(dirpath); // Free the memory allocated for the directory path
}

void create_directories(struct dir_indexes *dir_indexes, char *relpath, char **directories, int num_directories, struct flags *flags) {
    // A function that takes a directory index, a directory name, an array of directory names, and the number of directories, and creates placeholder directories in the directories that are not in the directory index
    struct index *current_dir_index = dir_indexes->head; // Loop through the directory indexes
    for (int i=0; i<num_directories; i++) {
        if (current_dir_index != NULL && i == current_dir_index->index) {
            // If the current directory index is not NULL and the current index is the same as the current directory index, go to the next directory index (so it skips the current index and will move onto checking for the next index in the linked list)
            current_dir_index = current_dir_index->next;
            continue; // Don't create the subdirectory as it already exists
        }
        create_directory(relpath, directories[i], flags); // Create the subdirectory in the current directory
    }
}