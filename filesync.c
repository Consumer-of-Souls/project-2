#include "mysync.h"

void copy_files(char *master_path, long long int master_size, char **filepaths, int num_filepaths, struct flags *flags) {
    // A function that takes a master file and an array of filepaths and copies the master file to each of the filepaths
    if (!flags->no_sync_flag) {
        // If the -n flag was not passed, copy the master file to each of the filepaths
        int master_fd = open(master_path, O_RDONLY); // Open the master file in read-only mode
        if (master_fd == -1) {
            // If open fails, print an error message and exit the program
            fprintf(stderr, "Error: could not open master file \"%s\"\n", master_path);
            exit(EXIT_FAILURE);
        }
        int *files = malloc_data(num_filepaths * sizeof(int)); // Allocate memory for the file descriptors
        for (int i = 0; i < num_filepaths; i++) {
            // Loop through the filepaths
            files[i] = open(filepaths[i], O_RDWR | O_CREAT | O_TRUNC, 0666); // Open the file in read-write mode, create it if it doesn't exist, and truncate it if it does exist, with permissions 0666
            if (files[i] == -1) {
                // If open fails, print an error message, close all the files that have been opened so far, and exit the program
                fprintf(stderr, "Error: could not open file \"%s\"\n", filepaths[i]);
                for (int j = 0; j < i; j++) {
                    close(files[j]);
                }
                free(files);
                close(master_fd);
                exit(EXIT_FAILURE);
            }
        }
        int page_size = sysconf(_SC_PAGESIZE); // Get the page size
        size_t buffer_size = master_size < page_size * 16 ? master_size : page_size * 16; // Set the buffer size to the master file size if it is less than 16 pages, otherwise set it to 16 pages (for efficiency)
        char *buffer = malloc_data(buffer_size); // Allocate memory for the buffer
        ssize_t bytes_read;
        while ((bytes_read = read(master_fd, buffer, buffer_size)) > 0) {
            // Loop through the master file and read it into the buffer
            for (int i = 0; i < num_filepaths; i++) {
                // Loop through the filepaths and write the buffer to each of the files (so that the master file is copied to each of the files, with only one loop through the master file)
                write(files[i], buffer, bytes_read);
            }
        }
        // Free the memory allocated for the buffer and close all the files
        free(buffer);
        close(master_fd);
        for (int i = 0; i < num_filepaths; i++) {
            close(files[i]);
        }
        free(files);
    }
    // Print a message for each of the files that have been copied
    for (int i=0; i<num_filepaths; i++) {
        VERBOSE_PRINT("Copied master file \"%s\" to file \"%s\"\n", master_path, filepaths[i]);
    }
}

void sync_master(struct file *master, char *relpath, char **directories, int num_directories, struct flags *flags) {
    // A function that takes a master file, a relative path to the file, an array of directory names, the number of directories, and a flags struct, and copies the master file to each of the directories
    char *master_path = malloc_data(strlen(directories[master->directory_index]) + strlen(relpath) + 2); // Allocate memory for the master file path
    sprintf(master_path, "%s/%s", directories[master->directory_index], relpath); // Create the master file path by concatenating the directory name and the relative path
    char **filepaths = malloc_data((num_directories-1) * sizeof(char *)); // Allocate memory for the filepaths
    int passed_master = 0; // A variable that keeps track of whether the master directory has been passed
    for (int i=0; i<num_directories; i++) {
        // Loop through the directories
        if (i == master->directory_index) {
            // If the current directory is the master directory, skip it and increment the passed_master variable
            passed_master = 1;
            continue;
        }
        filepaths[i - passed_master] = malloc_data(strlen(directories[i]) + strlen(relpath) + 2); // Allocate memory for the filepath
        sprintf(filepaths[i - passed_master], "%s/%s", directories[i], relpath); // Create the filepath by concatenating the directory name and the relative path
    }
    copy_files(master_path, master->size, filepaths, num_directories-1, flags); // Copy the master file to each of the filepaths
    if (flags->copy_perm_time_flag) {
        // If the -p flag was passed, set the permissions and modification time of each of the files to those of the master file
        struct utimbuf times; // Create a utimbuf struct
        times.modtime = master->edit_time; // Set the modification time to the modification time of the master file
        if (flags->verbose_flag) {
            // If the -v flag was passed, print the permissions and modification time of the master file
            char *readable_permissions = permissions(master->permissions);
            printf("Master file \"%s\" has permissions %s and modification time %lld\n", master_path, readable_permissions, master->edit_time);
        }
        for (int i=0; i<num_directories-1; i++) {
            // Loop through the filepaths and set the permissions and modification time of each of the files to those of the master file
            if (!flags->no_sync_flag) {
                // If the -n flag was not passed, set the permissions and modification time of the file
                if (utime(filepaths[i], &times) == -1) {
                    // If utime fails, print an error message and exit the program
                    fprintf(stderr, "Error: could not set modification time for file \"%s\"\n", filepaths[i]);
                    exit(EXIT_FAILURE);
                }
                if (chmod(filepaths[i], master->permissions) == -1) {
                    // If chmod fails, print an error message and exit the program
                    fprintf(stderr, "Error: could not set permissions for file \"%s\"\n", filepaths[i]);
                    exit(EXIT_FAILURE);
                }
            }
            // Print a message for each of the files that have had their permissions and modification time set and free the memory allocated for the filepath
            VERBOSE_PRINT("Set permissions for file \"%s\" to those of master file \"%s\"\n", filepaths[i], master_path);
            free(filepaths[i]);
        }
    } else {
        // If the -p flag was not passed, just free the memory allocated for the filepaths
        for (int i=0; i<num_directories-1; i++) {
            free(filepaths[i]);
        }
    }
    // Free the memory allocated for the master file path and the filepaths
    free(master_path);
    free(filepaths);
}
