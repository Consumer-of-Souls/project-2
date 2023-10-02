#include "mysync.h"

// 1. A linked list is created for each directory, and contains the files in that directory. These linked lists are stored in an array
// 2. The first file that can be (the first item of the linked list in the lowest array position) is chosen, and the file is checked against the linked lists later in the array to find files with the same name
// 2.5 If the file type for this file is a directory, the placeholder directories are all created, and the function is then called recursively with the locations of each directory (step 3 is skipped)
// 2.5 For the files with the same name, the newest version (the master file) and its directory index are stored
// 3. The newest version of the file, the index of its directory, and the directories are passed to a function that overwrites (or creates) the file with the master file's name in each directory that is not the master directory so it contains the master file's contents
// 4. Return to step 2 until the directory array is empty (all files have been synced)

// Clarification: if in different directories there are files with the same name, but different types (one is a directory, one is a file), what should happen? Are they skipped, or would one overwrite the other?
// Clarification: Will files still be overwritten if they contain the same contents as the master file?

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
    while ((opt = getopt(argc, argv, "ai:no:prv")) != -1) {
        switch (opt) {
            case 'a':
                flags->all_flag = 1;
                break;
            case 'i':
                enqueue_pattern(&(flags->ignore1), optarg);
                break;
            case 'n':
                flags->no_sync_flag = 1;
                flags->verbose_flag = 1;
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
    if (!check_directories(directories, num_directories, flags)) {
        fprintf(stderr, "Error: invalid directories specified\n");
        return 1;
    }
    sync_directories(directories, num_directories, flags);
    return 0;
}