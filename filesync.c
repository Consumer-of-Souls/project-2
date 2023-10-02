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

void copy_file(FILE *src, long long int size, char *destination, struct flags *flags) {
    fseek(src, 0, SEEK_SET);
    FILE *dest = fopen(destination, "wb");
    if (dest == NULL) {
        fprintf(stderr, "Error: could not open file %s\n", destination);
        exit(EXIT_FAILURE);
    }
    int page_size = sysconf(_SC_PAGESIZE);
    size_t buffer_size = size < page_size * 16 ? size : page_size * 16;
    char *buffer = malloc_data(buffer_size);
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, buffer_size, src)) > 0) {
        fwrite(buffer, 1, bytes_read, dest);
    }
    fclose(dest);
    free(buffer);
}

void sync_master(struct file *master, int master_index, char **directories, int num_directories, struct flags *flags) {
    // A function that takes a master file and an array of directory names and overwrites the file in each directory that shares the master file's name with the master file
    char *filename = master->name;
    char *master_path = malloc_data(strlen(directories[master_index]) + strlen(filename) + 2);
    sprintf(master_path, "%s/%s", directories[master_index], filename);
    FILE *master_file;
    if (!flags->no_sync_flag) {
        master_file = fopen(master_path, "rb");
    }
    if (flags->copy_perm_time_flag && flags->verbose_flag) {
        char *mode_string = permissions(master->permissions);
        VERBOSE_PRINT("Master file %s has permissions %s and modification time %lld\n", master_path, mode_string, master->edit_time);
    }
    for (int i = 0; i < num_directories; i++) {
        if (i != master_index) {
            // If the directory is not the master directory, overwrite the file in the directory with the master file
            char *filepath = malloc_data(strlen(directories[i]) + strlen(filename) + 2);
            sprintf(filepath, "%s/%s", directories[i], filename);
            if (!flags->no_sync_flag) {
                copy_file(master_file, master->size, filepath, flags);
            }
            VERBOSE_PRINT("Copied master file %s to %s\n", master_path, filepath);
            if (flags->copy_perm_time_flag) {
                struct utimbuf times;
                times.actime = master->edit_time;
                times.modtime = master->edit_time;
                if (!flags->no_sync_flag) {
                    utime(filepath, &times);
                    chmod(filepath, master->permissions);
                }
                VERBOSE_PRINT("Copied permissions and time from %s to %s\n", master_path, filepath);
            }
            free(filepath);
        }
    }
    free(master_path);
    if (!flags->no_sync_flag) {
        fclose(master_file);
    }
}
