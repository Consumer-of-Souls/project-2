#include "mysync.h"

struct hashtable *hashtable = NULL; // A hashtable that maps relative paths to either a struct file or a struct dir_indexes

struct relpaths *file_head = NULL; // A linked list of relative paths to files (for easy retrieval from the hashtable), the files are added in the order they are found, ensuring they are synced in the correct order (not as necessary as directories, but still useful)
struct relpaths *file_tail = NULL;

struct relpaths *dir_head = NULL; // A linked list of relative paths to directories (for easy retrieval from the hashtable), the directories are added in the order they are found, ensuring they are created in the correct order
struct relpaths *dir_tail = NULL;

void add_relpath(struct relpaths **head, struct relpaths **tail, char *relpath) {
    // A function that takes a head and tail pointer to a linked list of relative paths, and a relative path, and adds the relative path to the end of the linked list
    struct relpaths *new_relpath = malloc_data(sizeof(struct relpaths)); // Allocate memory for the new relative path
    new_relpath->relpath = strdup(relpath); // Copy the relative path into the new relative path
    new_relpath->next = NULL; // Set the next relative path to NULL
    if (*head == NULL) {
        // If the head is NULL (the linked list is empty), set the head and tail to the new relative path
        *head = new_relpath;
        *tail = new_relpath;
    } else {
        // If the head is not NULL (the linked list is not empty), set the next relative path of the tail to the new relative path, and set the tail to the new relative path
        (*tail)->next = new_relpath;
        *tail = new_relpath;
    }
}

bool read_directory(char *directory, char *base_dir, int base_dir_index, struct flags *flags) {
    // A function that takes a directory, a base directory, a base directory index, and a flags struct, and reads the directory, adding the files and directories to the hashtable
    bool found_files = false; // A bool that represents whether any files were found in the directory (initialised to false)
    DIR *dir = opendir(directory); // Open the directory
    if (dir == NULL) {
        // If the directory could not be opened, print an error message and exit the program
        fprintf(stderr, "Error: could not open directory \"%s\"\n", directory);
        exit(EXIT_FAILURE);
    }
    struct dirent *entry; // A struct that represents a directory entry
    struct stat file_info; // A struct that represents a file's info
    VERBOSE_PRINT("Reading directory \"%s\"\n", directory);
    while ((entry = readdir(dir)) != NULL) {
        // Loop through the directory entries
        char *filename = entry->d_name; // Get the filename
        if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
            // If the filename is "." or "..", skip it as it is not a file or directory
            continue;
        }
        char *filepath = malloc_data(strlen(directory) + strlen(filename) + 2); // Allocate memory for the filepath
        sprintf(filepath, "%s/%s", directory, filename); // Create the filepath by concatenating the directory and the filename
        if (stat(filepath, &file_info) == -1) {
            // If stat fails, print an error message and exit the program
            fprintf(stderr, "Error: could not get file info for file \"%s\"\n", filepath);
            exit(EXIT_FAILURE);
        }
        char *relpath = strdup(filepath + strlen(base_dir) + 1); // Create the relative path by copying the filepath, and removing the base directory from the start
        if (S_ISDIR(file_info.st_mode)) {
            // If the file is a directory
            if (!flags->recursive_flag) {
                // If the -r flag was not passed, skip the directory
                VERBOSE_PRINT("Skipping directory \"%s\"\n", filename);
                free(filepath);
                free(relpath);
                continue;
            }
            VERBOSE_PRINT("Found directory \"%s\"\n", filename);
            void *data = get(hashtable, relpath); // Check if the directory is already in the hashtable
            if (data == NULL) {
                add_relpath(&dir_head, &dir_tail, relpath); // Immediately add the directory to the linked list of directories (to ensure that the directories are added in the correct order) if it is not already in the hashtable
            }
            bool result = read_directory(filepath, base_dir, base_dir_index, flags); // Recursively read the directory, passing in the directory path, the base directory, the base directory index, and the flags struct
            found_files |= result; // Set the found_files variable to true if any files were found in the subdirectory (making the current directory not empty)
            if (data == NULL) {
                // If the directory is not already in the hashtable, add it to the hashtable
                struct dir_indexes *new_dir_indexes = malloc_data(sizeof(struct dir_indexes)); // Allocate memory for the new dir_indexes struct
                new_dir_indexes->type_id = 0; // Set the type_id to 0 (so it can be checked when casting)
                new_dir_indexes->valid = result; // Set the valid bool to the result of the recursive call
                struct index *new_index = malloc_data(sizeof(struct index)); // Allocate memory for the new index
                new_index->index = base_dir_index; // Set the index to the base directory index
                new_index->next = NULL; // Set the next index to NULL
                new_dir_indexes->head = new_index; // Set the head of the linked list of indexes to the new index
                new_dir_indexes->tail = new_index; // Set the tail of the linked list of indexes to the new index
                put(&hashtable, relpath, new_dir_indexes); // Put the new dir_indexes struct into the hashtable
                VERBOSE_PRINT("Added directory \"%s\" to hashtable\n", relpath);
            } else {
                int type = *(int *)data; // Cast the data to an int to get the type
                if (type != 0) {
                    // If the type is not 0, the data is not a dir_indexes struct, so print an error message and exit the program
                    fprintf(stderr, "Error: key \"%s\" doesn't map to a directory\n", relpath);
                    exit(EXIT_FAILURE);
                }
                struct dir_indexes *current_dir_indexes = (struct dir_indexes *)data; // Cast the data to a dir_indexes struct
                current_dir_indexes->valid |= result; // If the result of the recursive call is true, set the valid bool to true
                struct index *new_index = malloc_data(sizeof(struct index)); // Allocate memory for the new index
                new_index->index = base_dir_index; // Set the index to the base directory index
                new_index->next = NULL; // Set the next index to NULL
                current_dir_indexes->tail->next = new_index; // Set the next index of the tail of the linked list of indexes to the new index
                current_dir_indexes->tail = new_index; // Set the tail of the linked list of indexes to the new index
                VERBOSE_PRINT("Added directory \"%s\"'s index to hashtable\n", relpath);
            }
        } else if (S_ISREG(file_info.st_mode)) {
            // If the file is a regular file
            if (filename[0] == '.' && !flags->all_flag) {
                // If the filename starts with a '.', and the -a flag was not passed, skip the file
                VERBOSE_PRINT("Skipping hidden file \"%s\"\n", filename);
                free(filepath);
                free(relpath);
                continue;
            }
            if (flags->ignore1 != NULL && check_patterns(flags->ignore1, filename)) {
                // If the file matches an ignore pattern, skip the file
                VERBOSE_PRINT("Skipping file \"%s\" as it matches an ignore pattern\n", filename);
                free(filepath);
                free(relpath);
                continue;
            }
            if (flags->only1 != NULL && !check_patterns(flags->only1, filename)) {
                // If the file does not match an only pattern, skip the file
                VERBOSE_PRINT("Skipping file \"%s\" as it does not match an only pattern\n", filename);
                free(filepath);
                free(relpath);
                continue;
            }
            VERBOSE_PRINT("Found file \"%s\"\n", filename);
            found_files = true; // Set the found_files variable to true (as a file was found in the directory)
            void *data = get(hashtable, relpath); // Check if the file is already in the hashtable
            if (data == NULL) {
                // If the file is not already in the hashtable, add it to the hashtable
                add_relpath(&file_head, &file_tail, relpath); // Immediately add the file to the linked list of files (to ensure that the files are synced in the correct order, although this is not as necessary as directories) if it is not already in the hashtable
                struct file *new_file = malloc_data(sizeof(struct file)); // Allocate memory for the new file
                new_file->type_id = 1; // Set the type_id to 1 (so it can be checked when casting)
                new_file->directory_index = base_dir_index; // Set the directory index to the base directory index
                new_file->size = file_info.st_size; // Set the size to the size of the file
                new_file->permissions = file_info.st_mode; // Set the permissions to the permissions of the file
                new_file->edit_time = file_info.st_mtime; // Set the edit time to the modification time of the file
                put(&hashtable, relpath, new_file); // Put the new file into the hashtable
                VERBOSE_PRINT("Added file \"%s\" to hashtable\n", relpath);
            } else {
                // If the file is already in the hashtable, check if the file is a newer version
                int type = *(int *)data; // Cast the data to an int to get the type
                if (type != 1) {
                    // If the type is not 1, the data is not a file struct, so print an error message and exit the program
                    fprintf(stderr, "Error: key \"%s\" doesn't map to a file\n", relpath);
                    exit(EXIT_FAILURE);
                }
                struct file *current_file = (struct file *)data; // Cast the data to a file struct
                if (file_info.st_mtime > current_file->edit_time) {
                    // If the modification time of the file is greater than the modification time of the file in the hashtable, update the file in the hashtable
                    current_file->size = file_info.st_size; // Update the size of the file
                    current_file->permissions = file_info.st_mode; // Update the permissions of the file
                    current_file->edit_time = file_info.st_mtime; // Update the modification time of the file
                    current_file->directory_index = base_dir_index; // Update the directory index of the file
                    VERBOSE_PRINT("Updated file \"%s\" in hashtable as it is a newer version\n", relpath);
                } else {
                    VERBOSE_PRINT("Didn't update file \"%s\" in hashtable as it is an older version\n", relpath);
                }
            }
        }
        // Free the memory allocated for the filepath and relative path
        free(filepath);
        free(relpath);
    }
    // Close the directory and return whether any files were found in the directory (important for the recursive calls)
    closedir(dir);
    return found_files;
}

void sync_directories(char **directories, int num_directories, struct flags *flags) {
    // A function that takes an array of directory names, the number of directories, and a flags struct, and syncs the directories
    hashtable = create_hashtable(DEFAULT_HASHTABLE_SIZE); // Create the hashtable
    for (int i=0; i<num_directories; i++) {
        // Loop through the directories
        read_directory(directories[i], directories[i], i, flags); // Read the current directory
    }
    if (flags->verbose_flag) {
        // If the -v flag was passed, print the directories and files found
        printf("Directories found:\n");
        print_all(hashtable, dir_head, directories);
        printf("Master files found:\n");
        print_all(hashtable, file_head, directories);
    }
    // Loop through the directory linked list
    struct relpaths *current_dir = dir_head; // Set the current directory to the head of the linked list
    struct relpaths *temp = NULL; // A temporary variable to store the next directory
    while (current_dir != NULL) {
        // Loop through the directory linked list
        struct dir_indexes *current_dir_indexes = (struct dir_indexes *)get(hashtable, current_dir->relpath); // Get the directory indexes from the hashtable and cast them to a dir_indexes struct
        if (current_dir_indexes->valid) {
            // If the directory is not empty, create the directories in the locations they don't exist (aren't in the directory indexes)
            create_directories(current_dir_indexes, current_dir->relpath, directories, num_directories, flags);
        }
        delete(&hashtable, current_dir->relpath); // Delete the directory from the hashtable
        temp = current_dir->next; // Store the next directory
        free(current_dir->relpath); // Free the memory allocated for the relative path
        free(current_dir); // Free the memory allocated for the directory
        current_dir = temp; // Set the current directory to the next directory
    }
    // Loop through the file linked list
    struct relpaths *current_file = file_head; // Set the current file to the head of the linked list
    while (current_file != NULL) {
        // Loop through the file linked list
        struct file *current_file_info = (struct file *)get(hashtable, current_file->relpath); // Get the file from the hashtable and cast it to a file struct
        VERBOSE_PRINT("Syncing file \"%s\"\n", current_file->relpath);
        sync_master(current_file_info, current_file->relpath, directories, num_directories, flags); // Sync the file
        delete(&hashtable, current_file->relpath); // Delete the file from the hashtable
        temp = current_file->next; // Store the next file
        free(current_file->relpath); // Free the memory allocated for the relative path
        free(current_file); // Free the memory allocated for the file
        current_file = temp; // Set the current file to the next file
    }
    VERBOSE_PRINT("All files synced\n");
    // Free the memory allocated for the hashtable
    free(hashtable->table);
    free(hashtable);
}