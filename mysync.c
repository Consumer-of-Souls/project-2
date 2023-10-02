#include "mysync.h"

void *malloc_data(size_t size) {
    // Allocates memory for data and checks if malloc fails
    void *new_data = malloc(size); // Allocate memory for the new data
    if (new_data == NULL) {
        // If malloc fails, print an error message and exit the program
        fprintf(stderr, "Error: Failed to allocate memory for new data\n");
        exit(EXIT_FAILURE);
    }
    return new_data;
}

// 1. A linked list is created for each directory, and contains the files in that directory. These linked lists are stored in an array
// 2. The first file that can be (the first item of the linked list in the lowest array position) is chosen, and the file is checked against the linked lists later in the array to find files with the same name
// 2.5 If the file type for this file is a directory, the placeholder directories are all created, and the function is then called recursively with the locations of each directory (step 3 is skipped)
// 2.5 For the files with the same name, the newest version (the master file) and its directory index are stored
// 3. The newest version of the file, the index of its directory, and the directories are passed to a function that overwrites (or creates) the file with the master file's name in each directory that is not the master directory so it contains the master file's contents
// 4. Return to step 2 until the directory array is empty (all files have been synced)

// Clarification: if in different directories there are files with the same name, but different types (one is a directory, one is a file), what should happen? Are they skipped, or would one overwrite the other?

void copy_file(FILE *src, long long int size, char *destination, struct flags *flags) {
    FILE *dest = fopen(destination, "wb");
    if (dest == NULL) {
        fclose(src);
        fprintf(stderr, "Error: could not open file %s\n", destination);
        exit(EXIT_FAILURE);
    }
    long int page_size = sysconf(_SC_PAGESIZE);
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
    FILE *master_file = fopen(master_path, "rb");
    for (int i = 0; i < num_directories; i++) {
        if (i != master_index) {
            // If the directory is not the master directory, overwrite the file in the directory with the master file
            char *filepath = malloc_data(strlen(directories[i]) + strlen(filename) + 2);
            sprintf(filepath, "%s/%s", directories[i], filename);
            copy_file(master_file, master->size, filepath, flags);
            free(filepath);
        }
    }
    free(master_path);
    fclose(master_file);
}

int check_patterns(struct pattern *head, char *filename) {
    // A function that takes a linked list of patterns and a filename, and returns 1 if the filename matches any of the patterns, and 0 otherwise
    struct pattern *current_pattern = head;
    while (current_pattern != NULL) {
        int err = regexec(&(current_pattern->regex), filename, 0, NULL, REG_EXTENDED | REG_NOSUB);
        if (err == 0) {
            return 1;
        } else if (err != REG_NOMATCH) {
            fprintf(stderr, "Error: could not execute regex\n");
            exit(EXIT_FAILURE);
        }
        current_pattern = current_pattern->next;
    }
    return 0;
}

struct file_node **create_directory_contents(char **directories, int num_directories, struct flags *flags) {
    struct file_node **directory_contents = malloc_data(num_directories * sizeof(struct file_node *));
    DIR *dir;
    struct dirent *entry;
    struct stat file_info;
    for (int i = 0; i < num_directories; i++) {
        dir = opendir(directories[i]);
        if (dir == NULL) {
            fprintf(stderr, "Error: could not open directory %s\n", directories[i]);
            exit(EXIT_FAILURE);
        }
        directory_contents[i] = NULL;
        while ((entry = readdir(dir)) != NULL) {
            // Check if the entry has a period before it (hidden file) and should only be included if the all flag is set
            char *filename = entry->d_name;
            if (filename[0] == '.' && !flags->all_flag) {
                continue;
            }
            // Ensure the entry doesn't satisfy the ignore patterns
            if (flags->ignore1 != NULL && check_patterns(flags->ignore1, filename)) {
                continue;
            }
            // Ensure the entry satisfies the only patterns
            if (flags->only1 != NULL && !check_patterns(flags->only1, filename)) {
                continue;
            }
            // Check if the file is a directory and should only be included if the recursive flag is set
            if (S_ISDIR(file_info.st_mode) && !flags->recursive_flag) {
                continue;
            }
            char *filepath = malloc_data(strlen(directories[i]) + strlen(filename) + 2);
            sprintf(filepath, "%s/%s", directories[i], filename);
            if (stat(filepath, &file_info) == -1) {
                fprintf(stderr, "Error: could not get file info for file %s\n", filepath);
                exit(EXIT_FAILURE);
            }
            struct file *new_file = malloc_data(sizeof(struct file));
            new_file->name = strdup(filename);
            if (S_ISDIR(file_info.st_mode)) {
                new_file->type = "directory";
            } else {
                new_file->type = "file";
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

int find_master(struct file **master_file, int dir_index, struct file_node ***directory_contents, int num_directories, int verbose_flag) {
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

void placeholder_dirs(char *dir_name, int dir_index, char **directories, struct file_node ***directory_contents, int num_directories, int verbose_flag) {
    for (int i = 0; i < num_directories; i++) {
        if (i < dir_index) {
            // If the directory is before the master directory, create a placeholder subdirectory in the directory
            int result = mkdir(dir_name, 0777);
            if (result == -1) {
                fprintf(stderr, "Error: could not create directory %s\n", dir_name);
                exit(EXIT_FAILURE);
            }
        } else if (i > dir_index) {
            // Check if the subdirectory exists in the current directory
            struct file_node *current_node = (*directory_contents)[i];
            while (current_node != NULL) {
                struct file *current_file = current_node->file;
                if (strcmp(dir_name, current_file->name) == 0) {
                    break;
                }
                current_node = current_node->next;
            }
            if (current_node == NULL) {
                // If the subdirectory does not exist in the current directory, create a placeholder subdirectory in the directory
                int result = mkdir(dir_name, 0777);
                if (result == -1) {
                    fprintf(stderr, "Error: could not create directory %s\n", dir_name);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
}

void sync_directories(char **directories, int num_directories, struct flags *flags) {
    // A function that takes an array of directory names, and syncs the files in those directories
    struct file_node **directory_contents = create_directory_contents(directories, num_directories, flags);
    for (int i = 0; i < num_directories; i++) {
        while (directory_contents[i] != NULL) {
            struct file *master_file = directory_contents[i]->file;
            if (strcmp(master_file->type, "directory") == 0) {
                // If the file is a directory, create a placeholder subdirectory in each directory that doesn't contain the subdirectory
                placeholder_dirs(master_file->name, i, directories, &directory_contents, num_directories, flags->verbose_flag);
                char **subdirectories = malloc_data(num_directories * sizeof(char *));
                for (int j = 0; j < num_directories; j++) {
                    subdirectories[j] = malloc_data(strlen(directories[j]) + strlen(master_file->name) + 2);
                    sprintf(subdirectories[j], "%s/%s", directories[j], master_file->name);
                }
                sync_directories(subdirectories, num_directories, flags);
            } else {
                int master_index = find_master(&master_file, i, &directory_contents, num_directories, flags->verbose_flag);
                sync_master(master_file, master_index, directories, num_directories, flags);
            }
            directory_contents[i] = directory_contents[i]->next;
        }
    }
}

void enqueue_pattern(struct pattern **head, char *glob) {
    // A function that takes a glob pattern and converts it to a regex pattern, and then compiles it and adds it to the linked list
    char *regex = glob2regex(glob);
    struct pattern *new_pattern = malloc_data(sizeof(struct pattern));
    int err = regcomp(&(new_pattern->regex), regex, REG_EXTENDED | REG_NOSUB);
    if (err != 0) {
        fprintf(stderr, "Error: could not compile regex %s\n", regex);
        exit(EXIT_FAILURE);
    }
    new_pattern->next = *head;
    *head = new_pattern;
}

int main(int argc, char **argv) {
    struct flags *flags = malloc_data(sizeof(struct flags));
    flags->all_flag = 0;
    flags->ignore1 = NULL;
    flags->no_sync_flag = 0;
    flags->only1 = NULL;
    flags->copy_perm_time_flag = 0;
    flags->recursive_flag = 0;
    flags->verbose_flag = 0;
    opterr = 0;
    int opt;
    while ((opt = getopt(argc, argv, "ainoprv")) != -1) {
        switch (opt) {
            case 'a':
                flags->all_flag = 1;
                break;
            case 'i':
                enqueue_pattern(&(flags->ignore1), optarg);
                break;
            case 'n':
                flags->no_sync_flag = 1;
                break;
            case 'o':
                enqueue_pattern(&(flags->only1), optarg);
                break;
            case 'p':
                flags->copy_perm_time_flag = 1;
                break;
            case 'r':
                flags->recursive_flag = 1;
                break;
            case 'v':
                flags->verbose_flag = 1;
                break;
            case '?':
                fprintf(stderr, "Unknown option -%c.\n", optopt);
                return 1;
            default:
                abort();
        }
    }
    int num_directories = argc - optind;
    if (num_directories < 2) {
        fprintf(stderr, "Error: not enough directories specified\n");
        return 1;
    }
    char **directories = malloc_data((num_directories) * sizeof(char *));
    for (int i = optind; i < argc; i++) {
        directories[i - optind] = strdup(argv[i]);
    }
    sync_directories(directories, num_directories, flags);
    return 0;
}