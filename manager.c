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

struct file_names *read_directories(struct hashtable **hashtable, char **directories, int num_directories, struct flags *flags) {
    // A function that takes a hashtable, an array of directory names, and the number of directories, and reads the directories into the hashtable, returning a linked list of all the file names
    struct file_names *all_names = NULL;
    DIR *dir;
    struct dirent *entry;
    struct stat file_info;
    for (int i=0; i<num_directories; i++) {
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
                VERBOSE_PRINT("Found directory %s\n", filename);
                void *data = get(*hashtable, filename);
                if (data == NULL) {
                    // If the directory does not exist in the hashtable, add it to the hashtable
                    struct dir_indexes *new_dir_indexes = malloc_data(sizeof(struct dir_indexes));
                    new_dir_indexes->type_id = 0;
                    struct index *new_index = malloc_data(sizeof(struct index));
                    new_index->index = i;
                    new_index->next = NULL;
                    new_dir_indexes->head = new_index;
                    new_dir_indexes->tail = new_index;
                    put(hashtable, filename, (void *)new_dir_indexes);
                    struct file_names *new_file_name = malloc_data(sizeof(struct file_names));
                    new_file_name->name = strdup(filename);
                    new_file_name->next = all_names;
                    all_names = new_file_name;
                    VERBOSE_PRINT("Added directory %s to hashtable\n", filename);
                } else {
                    // If the key exists, check if the data is a dir_index struct
                    int type = *(int *)data;
                    if (type != 0) {
                        fprintf(stderr, "Error: key \"%s\" doesn't map to a directory\n", filename);
                        exit(EXIT_FAILURE);
                    }
                    // If the data is a dir_index struct, add the directory index to the linked list
                    struct dir_indexes *current_dir_indexes = (struct dir_indexes *)data;
                    struct index *new_index = malloc_data(sizeof(struct index));
                    new_index->index = i;
                    new_index->next = NULL;
                    current_dir_indexes->tail->next = new_index;
                    current_dir_indexes->tail = new_index;
                    VERBOSE_PRINT("Added directory %s's index to hashtable\n", filename);
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
                // Get the data for the current key from the hashtable
                VERBOSE_PRINT("Found file \"%s\"\n", filename);
                void *data = get(*hashtable, filename);
                if (data == NULL) {
                    // If the file does not exist in the hashtable, add it to the hashtable
                    struct file *new_file = malloc_data(sizeof(struct file));
                    new_file->type_id = 1;
                    new_file->permissions = file_info.st_mode;
                    new_file->edit_time = file_info.st_mtime;
                    new_file->size = file_info.st_size;
                    new_file->directory_index = i;
                    put(hashtable, filename, (void *)new_file);
                    // Add the filename to the linked list
                    struct file_names *new_file_name = malloc_data(sizeof(struct file_names));
                    new_file_name->name = strdup(filename);
                    new_file_name->next = all_names;
                    all_names = new_file_name;
                    VERBOSE_PRINT("Added file \"%s\" to hashtable\n", filename);
                } else {
                    // If the key exists, check if the data is a file struct
                    int type = *(int *)data;
                    if (type != 1) {
                        fprintf(stderr, "Error: key \"%s\" doesn't map to a file\n", filename);
                        exit(EXIT_FAILURE);
                    }
                    // If the data is a file struct, check if the edit time is newer
                    struct file *current_file = (struct file *)data;
                    if (file_info.st_mtime > current_file->edit_time) {
                        // If the edit time is newer, replace the file in the hashtable
                        current_file->permissions = file_info.st_mode;
                        current_file->edit_time = file_info.st_mtime;
                        current_file->size = file_info.st_size;
                        current_file->directory_index = i;
                        VERBOSE_PRINT("Replaced file \"%s\" in hashtable as the edit time is newer\n", filename);
                    } else {
                        VERBOSE_PRINT("Did not replace file \"%s\" in hashtable as the edit time is not newer\n", filename);
                    }
                }
            } 
        }
    }
    return all_names;
}

void sync_directories(char **directories, int num_directories, struct flags *flags) {
    // A function that takes an array of directory names, and syncs the files in those directories
    // Create a hashtable that is updated to contain the newest files for each filename and all of the subdirectories and the directories the subdirectories are already in
    struct hashtable *hashtable = create_hashtable(100);
    struct file_names *all_names = read_directories(&hashtable, directories, num_directories, flags);
    VERBOSE_PRINT("Finished reading directories\n");
    // Now that we have the hashtable of the newest files and the location of the subdirectories, we can sync the files
    if (flags->verbose_flag) {
        print_all(hashtable, all_names, directories);
    }
    while (all_names != NULL) {
        char *filename = all_names->name;
        // Get the data for the current key from the hashtable
        void *data = get(hashtable, filename);
        if (data == NULL) {
            fprintf(stderr, "Error: data for key \"%s\" is NULL\n", filename);
            exit(EXIT_FAILURE);
        }
        int type = *(int *)data;
        if (type == 1) {
            // If the data is a file struct, sync the file
            struct file *current_file = (struct file *)data;
            VERBOSE_PRINT("Syncing file \"%s\"\n", filename);
            sync_master(current_file, filename, directories, num_directories, flags);
            delete(&hashtable, filename);
        } else if (type == 0) {
            // If the data is a dir_index struct, sync the directory
            struct dir_indexes *current_dir_indexes = (struct dir_indexes *)data;
            VERBOSE_PRINT("Syncing directory \"%s\"\n", filename);
            placeholder_dirs(current_dir_indexes, filename, directories, num_directories, flags);
            delete(&hashtable, filename);
            // Create a new array of subdirectories
            char **subdirectories = malloc_data((num_directories) * sizeof(char *));
            for (int i=0; i<num_directories; i++) {
                subdirectories[i] = malloc_data(strlen(directories[i]) + strlen(filename) + 2);
                sprintf(subdirectories[i], "%s/%s", directories[i], filename);
            }
            // Recursively sync the subdirectories
            sync_directories(subdirectories, num_directories, flags);
            VERBOSE_PRINT("Finished syncing subdirectory \"%s\"\n", filename);
        } else {
            fprintf(stderr, "Error: data for key \"%s\" is not a file struct or a dir_index struct\n", filename);
            exit(EXIT_FAILURE);
        }
        struct file_names *next_file_name = all_names->next;
        free(all_names->name);
        free(all_names);
        all_names = next_file_name;
    }
    free(hashtable->table);
    free(hashtable);
}