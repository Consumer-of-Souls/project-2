#include "mysync.h"

struct hashtable *hashtable = NULL; // A hashtable that maps relative paths to either a struct file or a struct dir_indexes
struct relpaths *file_head = NULL; // A linked list of relative paths to files (for easy retrieval from the hashtable), the files are added in the order they are found, ensuring they are synced in the correct order (not as necessary as directories, but still useful)
struct relpaths *file_tail = NULL;
struct relpaths *dir_head = NULL; // A linked list of relative paths to directories (for easy retrieval from the hashtable), the directories are added in the order they are found, ensuring they are created in the correct order
struct relpaths *dir_tail = NULL;

void add_relpath(struct relpaths **head, struct relpaths **tail, char *relpath) {
    // A function that takes a head and tail pointer to a linked list of relative paths, and a relative path, and adds the relative path to the end of the linked list
    struct relpaths *new_relpath = malloc_data(sizeof(struct relpaths));
    new_relpath->relpath = strdup(relpath);
    new_relpath->next = NULL;
    if (*head == NULL) {
        *head = new_relpath;
        *tail = new_relpath;
    } else {
        (*tail)->next = new_relpath;
        *tail = new_relpath;
    }
}

bool read_directory(char *directory, char *base_dir, int base_dir_index, struct flags *flags) {
    bool found_files = false;
    DIR *dir = opendir(directory);
    if (dir == NULL) {
        fprintf(stderr, "Error: could not open directory \"%s\"\n", directory);
        exit(EXIT_FAILURE);
    }
    struct dirent *entry;
    struct stat file_info;
    VERBOSE_PRINT("Reading directory \"%s\"\n", directory);
    while ((entry = readdir(dir)) != NULL) {
        char *filename = entry->d_name;
        if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
            continue;
        }
        char *filepath = malloc_data(strlen(directory) + strlen(filename) + 2);
        sprintf(filepath, "%s/%s", directory, filename);
        if (stat(filepath, &file_info) == -1) {
            fprintf(stderr, "Error: could not get file info for file \"%s\"\n", filepath);
            exit(EXIT_FAILURE);
        }
        char *relpath = strdup(filepath + strlen(base_dir) + 1);
        if (S_ISDIR(file_info.st_mode)) {
            if (!flags->recursive_flag) {
                VERBOSE_PRINT("Skipping directory \"%s\"\n", filename);
                free(filepath);
                free(relpath);
                continue;
            }
            VERBOSE_PRINT("Found directory \"%s\"\n", filename);
            void *data = get(hashtable, relpath);
            if (data == NULL) {
                add_relpath(&dir_head, &dir_tail, relpath); // Add the directory to the linked list of directories (to ensure that the directories are added in the correct order)
            }
            bool result = read_directory(filepath, base_dir, base_dir_index, flags);
            found_files |= result;
            if (data == NULL) {
                struct dir_indexes *new_dir_indexes = malloc_data(sizeof(struct dir_indexes));
                new_dir_indexes->type_id = 0;
                new_dir_indexes->valid = result;
                struct index *new_index = malloc_data(sizeof(struct index));
                new_index->index = base_dir_index;
                new_index->next = NULL;
                new_dir_indexes->head = new_index;
                new_dir_indexes->tail = new_index;
                put(&hashtable, relpath, new_dir_indexes);
                VERBOSE_PRINT("Added directory \"%s\" to hashtable\n", relpath);
            } else {
                int type = *(int *)data;
                if (type != 0) {
                    fprintf(stderr, "Error: key \"%s\" doesn't map to a directory\n", relpath);
                    exit(EXIT_FAILURE);
                }
                struct dir_indexes *current_dir_indexes = (struct dir_indexes *)data;
                if (result) {
                    current_dir_indexes->valid = true;
                }
                struct index *new_index = malloc_data(sizeof(struct index));
                new_index->index = base_dir_index;
                new_index->next = NULL;
                current_dir_indexes->tail->next = new_index;
                current_dir_indexes->tail = new_index;
                VERBOSE_PRINT("Added directory \"%s\"'s index to hashtable\n", relpath);
            }
        } else if (S_ISREG(file_info.st_mode)) {
            if (filename[0] == '.' && !flags->all_flag) {
                VERBOSE_PRINT("Skipping hidden file \"%s\"\n", filename);
                continue;
            }
            if (flags->ignore1 != NULL && check_patterns(flags->ignore1, filename)) {
                VERBOSE_PRINT("Skipping file \"%s\" as it matches an ignore pattern\n", filename);
                free(filepath);
                free(relpath);
                continue;
            }
            // Ensure the entry satisfies the only patterns
            if (flags->only1 != NULL && !check_patterns(flags->only1, filename)) {
                VERBOSE_PRINT("Skipping file \"%s\" as it does not match an only pattern\n", filename);
                free(filepath);
                free(relpath);
                continue;
            }
            VERBOSE_PRINT("Found file \"%s\"\n", filename);
            found_files = true;
            void *data = get(hashtable, relpath);
            if (data == NULL) {
                add_relpath(&file_head, &file_tail, relpath);
                struct file *new_file = malloc_data(sizeof(struct file));
                new_file->type_id = 1;
                new_file->directory_index = base_dir_index;
                new_file->size = file_info.st_size;
                new_file->permissions = file_info.st_mode;
                new_file->edit_time = file_info.st_mtime;
                put(&hashtable, relpath, new_file);
                VERBOSE_PRINT("Added file \"%s\" to hashtable\n", relpath);
            } else {
                int type = *(int *)data;
                if (type != 1) {
                    fprintf(stderr, "Error: key \"%s\" doesn't map to a file\n", relpath);
                    exit(EXIT_FAILURE);
                }
                struct file *current_file = (struct file *)data;
                if (file_info.st_mtime > current_file->edit_time) {
                    current_file->size = file_info.st_size;
                    current_file->permissions = file_info.st_mode;
                    current_file->edit_time = file_info.st_mtime;
                    current_file->directory_index = base_dir_index;
                    VERBOSE_PRINT("Updated file \"%s\" in hashtable as it is a newer version\n", relpath);
                } else {
                    VERBOSE_PRINT("Didn't update file \"%s\" in hashtable as it is an older version\n", relpath);
                }
            }
        }
        free(filepath);
        free(relpath);
    }
    closedir(dir);
    return found_files;
}

void sync_directories(char **directories, int num_directories, struct flags *flags) {
    hashtable = create_hashtable(DEFAULT_HASHTABLE_SIZE);
    for (int i=0; i<num_directories; i++) {
        read_directory(directories[i], directories[i], i, flags);
    }
    if (flags->verbose_flag) {
        printf("Directories found:\n");
        print_all(hashtable, dir_head, directories);
        printf("Master files found:\n");
        print_all(hashtable, file_head, directories);
    }
    // Loop through the directory linked list
    struct relpaths *current_dir = dir_head;
    struct relpaths *temp = NULL;
    while (current_dir != NULL) {
        struct dir_indexes *current_dir_indexes = (struct dir_indexes *)get(hashtable, current_dir->relpath);
        if (current_dir_indexes->valid) {
            placeholder_dirs(current_dir_indexes, current_dir->relpath, directories, num_directories, flags);
        }
        delete(&hashtable, current_dir->relpath);
        temp = current_dir->next;
        free(current_dir->relpath);
        free(current_dir);
        current_dir = temp;
    }
    // Loop through the file linked list
    struct relpaths *current_file = file_head;
    while (current_file != NULL) {
        struct file *current_file_info = (struct file *)get(hashtable, current_file->relpath);
        VERBOSE_PRINT("Syncing file \"%s\"\n", current_file->relpath);
        sync_master(current_file_info, current_file->relpath, directories, num_directories, flags);
        delete(&hashtable, current_file->relpath);
        temp = current_file->next;
        free(current_file->relpath);
        free(current_file);
        current_file = temp;
    }
    VERBOSE_PRINT("All files synced\n");
    free(hashtable->table);
    free(hashtable);
}