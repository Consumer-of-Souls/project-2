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
        VERBOSE_PRINT("Reading directory %s:\n", directories[i]);
        if (flags->no_sync_flag && access(directories[i], F_OK) == -1) {
            directory_contents[i] = NULL;
            continue;
        }
        dir = opendir(directories[i]);
        if (dir == NULL) {
            fprintf(stderr, "Error: could not open directory %s\n", directories[i]);
            exit(EXIT_FAILURE);
        }
        directory_contents[i] = NULL;
        while ((entry = readdir(dir)) != NULL) {
            // Check if the entry has a period before it (hidden file) and should only be included if the all flag is set
            char *filename = entry->d_name;
            if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
                continue;
            }
            if (filename[0] == '.' && !flags->all_flag) {
                VERBOSE_PRINT("Skipping hidden file %s\n", filename);
                continue;
            }
            // Ensure the entry doesn't satisfy the ignore patterns
            if (flags->ignore1 != NULL && check_patterns(flags->ignore1, filename)) {
                VERBOSE_PRINT("Skipping file %s as it matches an ignore pattern\n", filename);
                continue;
            }
            // Ensure the entry satisfies the only patterns
            if (flags->only1 != NULL && !check_patterns(flags->only1, filename)) {
                VERBOSE_PRINT("Skipping file %s as it does not match an only pattern\n", filename);
                continue;
            }
            
            char *filepath = malloc_data(strlen(directories[i]) + strlen(filename) + 2);
            sprintf(filepath, "%s/%s", directories[i], filename);
            if (stat(filepath, &file_info) == -1) {
                fprintf(stderr, "Error: could not get file info for file %s\n", filepath);
                exit(EXIT_FAILURE);
            }
            
            // Check if the file is a directory and should only be included if the recursive flag is set
            if (S_ISDIR(file_info.st_mode) && !flags->recursive_flag) {
                VERBOSE_PRINT("Skipping directory %s\n", filename);
                continue;
            }
            struct file *new_file = malloc_data(sizeof(struct file));
            new_file->name = strdup(filename);
            if (S_ISDIR(file_info.st_mode)) {
                new_file->type = "directory";
                VERBOSE_PRINT("Added subdirectory %s to directory %s contents\n", filename, directories[i]);
            } else {
                new_file->type = "file";
                VERBOSE_PRINT("Added file %s to directory %s contents\n", filename, directories[i]);
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
    struct file_node **directory_contents = create_directory_contents(directories, num_directories, flags);
    if (flags->verbose_flag) {
        print_directories(directory_contents, directories, num_directories);
    }
    for (int i = 0; i < num_directories; i++) {
        while (directory_contents[i] != NULL) {
            struct file *master_file = directory_contents[i]->file;
            if (strcmp(master_file->type, "directory") == 0) {
                // If the file is a directory, create a placeholder subdirectory in each directory that doesn't contain the subdirectory
                placeholder_dirs(master_file->name, i, directories, &directory_contents, num_directories, flags);
                char **subdirectories = malloc_data(num_directories * sizeof(char *));
                for (int j = 0; j < num_directories; j++) {
                    subdirectories[j] = malloc_data(strlen(directories[j]) + strlen(master_file->name) + 2);
                    sprintf(subdirectories[j], "%s/%s", directories[j], master_file->name);
                }
                sync_directories(subdirectories, num_directories, flags);
            } else {
                VERBOSE_PRINT("Syncing file %s\n", master_file->name);
                int master_index = find_master(&master_file, i, &directory_contents, num_directories);
                VERBOSE_PRINT("Master file for %s is in directory %s\n", master_file->name, directories[master_index]);
                sync_master(master_file, master_index, directories, num_directories, flags);
            }
            directory_contents[i] = directory_contents[i]->next;
        }
    }
}