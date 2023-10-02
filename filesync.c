#include "mysync.h"

int find_master(struct file **master_file, int dir_index, struct file_node ***directory_contents, int num_directories) {
    // A function that takes a file and an array of directory contents and returns the index of the newest version of the file
    int master_index = dir_index;
    for (int i = dir_index + 1; i < num_directories; i++) {
        struct file_node *current_node = (*directory_contents)[i];
        struct file_node *prev_node = NULL;
        while (current_node != NULL) {
            struct file *current_file = current_node->file;
            if (strcmp((*master_file)->name, current_file->name) == 0) {
                if ((*master_file)->edit_time < current_file->edit_time) {
                    *master_file = current_file;
                    master_index = i;
                }
                // Remove the file from the linked list, as it has been found
                if (prev_node == NULL) {
                    (*directory_contents)[i] = current_node->next;
                } else {
                    prev_node->next = current_node->next;
                }
                break; // Break out of the loop, as the file has been found (will move to the next directory)
            }
            prev_node = current_node;
            current_node = current_node->next;
        }
    }
    return master_index;
}

void copy_files(char *master_path, long long int master_size, char **filepaths, int num_directories, struct flags *flags) {
    if (!flags->no_sync_flag) {
        FILE *master_file = fopen(master_path, "rb");
        if (master_file == NULL) {
            fprintf(stderr, "Error: could not open file %s\n", master_path);
            exit(EXIT_FAILURE);
        }
        FILE **files = malloc_data(num_directories * sizeof(FILE *));
        for (int i = 0; i < num_directories; i++) {
            files[i] = fopen(filepaths[i], "wb");
            if (files[i] == NULL) {
                fprintf(stderr, "Error: could not open file %s\n", filepaths[i]);
                exit(EXIT_FAILURE);
            }
        }
        int page_size = sysconf(_SC_PAGESIZE);
        size_t buffer_size = master_size < page_size * 16 ? master_size : page_size * 16;
        char *buffer = malloc_data(buffer_size);
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, buffer_size, master_file)) > 0) {
            for (int i = 0; i < num_directories; i++) {
                fwrite(buffer, 1, bytes_read, files[i]);
            }
        }
    }
    for (int i=0; i<num_directories; i++) {
        VERBOSE_PRINT("Copied master file \"%s\" to file \"%s\"\n", master_path, filepaths[i]);
    }
}

void sync_master(struct file *master, int master_index, char **directories, int num_directories, struct flags *flags) {
    // A function that takes a master file and an array of directory names and overwrites the file in each directory that shares the master file's name with the master file
    char *filename = master->name;
    char *master_path = malloc_data(strlen(directories[master_index]) + strlen(filename) + 2);
    sprintf(master_path, "%s/%s", directories[master_index], filename);
    char **filepaths = malloc_data((num_directories-1) * sizeof(char *));
    int passed_master = 0;
    for (int i=0; i<num_directories; i++) {
        if (i == master_index) {
            passed_master = 1;
            continue;
        }
        filepaths[i - passed_master] = malloc_data(strlen(directories[i]) + strlen(filename) + 2);
        sprintf(filepaths[i - passed_master], "%s/%s", directories[i], filename);
    }
    free(filename);
    copy_files(master_path, master->size, filepaths, num_directories-1, flags);
    if (flags->copy_perm_time_flag) {
        char *readable_permissions;
        if (flags->verbose_flag) {
            readable_permissions = permissions(master->permissions);
        }
        struct utimbuf times;
        times.actime = master->edit_time;
        times.modtime = master->edit_time;
        VERBOSE_PRINT("Master file \"%s\" has permissions %s and modification time %lld\n", master_path, readable_permissions, master->edit_time);
        for (int i=0; i<num_directories-1; i++) {
            if (!flags->no_sync_flag) {
                if (utime(filepaths[i], &times) == -1) {
                    fprintf(stderr, "Error: could not set modification time for file \"%s\"\n", filepaths[i]);
                    exit(EXIT_FAILURE);
                }
                if (chmod(filepaths[i], master->permissions) == -1) {
                    fprintf(stderr, "Error: could not set permissions for file \"%s\"\n", filepaths[i]);
                    exit(EXIT_FAILURE);
                }
            }
            VERBOSE_PRINT("Set permissions for file \"%s\" to those of master file \"%s\"\n", filepaths[i], master_path);
            free(filepaths[i]);
        }
    }
    free(master_path);
    free(filepaths);
}
