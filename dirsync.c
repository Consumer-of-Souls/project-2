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

void placeholder_dirs(char *dir_name, int dir_index, char **directories, struct file_node ***directory_contents, int num_directories, struct flags *flags) {
    for (int i = 0; i < num_directories; i++) {
        if (i < dir_index) {
            // If the directory is before the master directory, create a placeholder subdirectory in the directory
            create_directory(dir_name, directories[i], flags);
        } else if (i > dir_index) {
            // Check if the subdirectory exists in the current directory
            struct file_node *current_node = (*directory_contents)[i];
            struct file_node *prev_node = NULL;
            while (current_node != NULL) {
                struct file *current_file = current_node->file;
                if (strcmp(dir_name, current_file->name) == 0) {
                    // Remove the subdirectory from the linked list, as it has been found
                    if (prev_node == NULL) {
                        (*directory_contents)[i] = current_node->next;
                    } else {
                        prev_node->next = current_node->next;
                    }
                    break;
                }
                prev_node = current_node;
                current_node = current_node->next;
            }
            if (current_node == NULL) {
                // If the subdirectory does not exist in the current directory, create a placeholder subdirectory in the directory
                create_directory(dir_name, directories[i], flags);
            }
        }
    }
}
