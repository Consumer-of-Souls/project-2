#include "mysync.h"

void create_directory(char *dir_name, char *parent_dir, struct flags *flags) {
    // A function that takes a directory name and a parent directory name, and creates the directory in the parent directory
    char *dirpath = malloc_data(strlen(parent_dir) + strlen(dir_name) + 2);
    sprintf(dirpath, "%s/%s", parent_dir, dir_name);
    if (!flags->no_sync_flag) {
        int result = mkdir(dirpath, 0777);
        if (result == -1) {
            fprintf(stderr, "Error: could not create directory %s\n", dirpath);
            exit(EXIT_FAILURE);
        }
    }
    VERBOSE_PRINT("Created directory %s as it did not exist\n", dirpath);
    free(dirpath);
}

void placeholder_dirs(struct dir_indexes *dir_indexes, char *dir_name, char **directories, int num_directories, struct flags *flags) {
    // A function that takes a directory index, a directory name, an array of directory names, and the number of directories, and creates placeholder directories in the directories that are not in the directory index
    struct index *current_dir_index = dir_indexes->head;
    for (int i=0; i<num_directories; i++) {
        if (current_dir_index != NULL && i == current_dir_index->index) {
            current_dir_index = current_dir_index->next;
            continue;
        }
        create_directory(dir_name, directories[i], flags);
    }
}