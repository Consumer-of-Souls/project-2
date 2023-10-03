#include "mysync.h"

int check_directories(char **directories, int num_directories, struct flags *flags) {
    // A function that takes an array of directory names, and returns 1 if all the directories exist, and 0 otherwise
    for (int i = 0; i < num_directories; i++) {
        if (access(directories[i], F_OK) == -1) {
            VERBOSE_PRINT("Directory %s does not exist\n", directories[i]);
            return 0;
        }
    }
    VERBOSE_PRINT("All directories exist\n");
    return 1;
}

struct file_node **create_directory_contents(char **directories, int num_directories, struct flags *flags) {
    struct file_node **directory_contents = malloc_data(num_directories * sizeof(struct file_node *));
    DIR *dir;
    struct dirent *entry;
    struct stat file_info;
    for (int i = 0; i < num_directories; i++) {
        directory_contents[i] = NULL;
        VERBOSE_PRINT("Reading directory %s\n", directories[i]);
        if (flags->no_sync_flag && access(directories[i], F_OK) == -1) {
            continue;
        }
        dir = opendir(directories[i]);
        if (dir == NULL) {
            fprintf(stderr, "Error: could not open directory %s\n", directories[i]);
            exit(EXIT_FAILURE);
        }
        while ((entry = readdir(dir)) != NULL) {
            char *filename = entry->d_name;
            if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
                continue;
            }
            if (filename[0] == '.' && !flags->all_flag) {
                VERBOSE_PRINT("Skipping hidden file \"%s\"\n", filename);
                continue;
            }
            char *filepath = malloc_data(strlen(directories[i]) + strlen(filename) + 2);
            sprintf(filepath, "%s/%s", directories[i], filename);
            if (stat(filepath, &file_info) == -1) {
                fprintf(stderr, "Error: could not get file info for file \"%s\"\n", filepath);
                exit(EXIT_FAILURE);
            }
            free(filepath);
            if (S_ISDIR(file_info.st_mode)) {
                if (!flags->recursive_flag) {
                    VERBOSE_PRINT("Skipping directory %s\n", filename);
                    continue;
                }
            } else {
                // Ensure the entry doesn't satisfy the ignore patterns
                if (flags->ignore1 != NULL && check_patterns(flags->ignore1, filename)) {
                    VERBOSE_PRINT("Skipping file \"%s\" as it matches an ignore pattern\n", filename);
                    continue;
                }
                // Ensure the entry satisfies the only patterns
                if (flags->only1 != NULL && !check_patterns(flags->only1, filename)) {
                    VERBOSE_PRINT("Skipping file \"%s\" as it does not match an only pattern\n", filename);
                    continue;
                }
            } 
            struct file *new_file = malloc_data(sizeof(struct file));
            new_file->name = strdup(filename);
            if (S_ISDIR(file_info.st_mode)) {
                new_file->type = strdup("directory");
                VERBOSE_PRINT("Added subdirectory %s to directory %s contents\n", filename, directories[i]);
            } else {
                new_file->type = strdup("file");
                VERBOSE_PRINT("Added file \"%s\" to directory %s contents\n", filename, directories[i]);
            }
            new_file->permissions = file_info.st_mode;
            new_file->edit_time = file_info.st_mtime;
            new_file->size = file_info.st_size;
            struct file_node *new_node = malloc_data(sizeof(struct file_node));
            new_node->file = new_file;
            new_node->next = directory_contents[i];
            directory_contents[i] = new_node;
        }
        closedir(dir);
    }
    return directory_contents;
}

void sync_directories(char **directories, int num_directories, struct flags *flags) {
    // A function that takes an array of directory names, and syncs the files in those directories
    
    // The contents of the directories are stored in an array of linked lists of file_nodes, with each array index corresponding to a directory
    struct file_node **directory_contents = create_directory_contents(directories, num_directories, flags);
    if (flags->verbose_flag) {
        // Print the contents of each directory
        print_directories(directory_contents, directories, num_directories);
    }
    // Iterate through each directory (each array index)
    for (int i = 0; i < num_directories; i++) {
        // For each directory, iterate through the linked list of file_nodes
        while (directory_contents[i] != NULL) {
            // For each file_node, sync the file or subdirectory
            struct file *master_file = directory_contents[i]->file; // The file or subdirectory to sync
            if (strcmp(master_file->type, "directory") == 0) {
                // If the file is a directory, create a placeholder subdirectory in each directory that doesn't already contain it
                placeholder_dirs(master_file->name, i, directories, &directory_contents, num_directories, flags);
                char **subdirectories = malloc_data(num_directories * sizeof(char *)); // Allocate an array of subdirectory names
                for (int j = 0; j < num_directories; j++) {
                    // For each directory, create a subdirectory name by concatenating the directory name with the master directory name
                    subdirectories[j] = malloc_data(strlen(directories[j]) + strlen(master_file->name) + 2);
                    sprintf(subdirectories[j], "%s/%s", directories[j], master_file->name);
                }
                sync_directories(subdirectories, num_directories, flags); // Recursively sync the subdirectories
                // Free the subdirectory names
                for (int j = 0; j < num_directories; j++) {
                    free(subdirectories[j]);
                }
                free(subdirectories);
            } else {
                VERBOSE_PRINT("Syncing file \"%s\"\n", master_file->name);
                // Find the index of the directory that contains the master file and the master file itself
                int master_index = find_master(&master_file, i, &directory_contents, num_directories);
                VERBOSE_PRINT("Master file for \"%s\" is in directory %s\n", master_file->name, directories[master_index]);
                // Sync the master file by overwriting the other files with the same name in the other directories
                sync_master(master_file, master_index, directories, num_directories, flags);
            }
            // Free the master file and the file_node as it has been synced
            free_file(master_file);
            free(directory_contents[i]);
            directory_contents[i] = directory_contents[i]->next;
        }
    }
    // Free the array used to store the contents of the directories (now empty)
    free(directory_contents);
    VERBOSE_PRINT("Finished syncing files and subdirectories within the current directory\n");
}