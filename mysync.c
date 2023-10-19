#include "mysync.h"

// 1. Scan each directory passed in the command line arguments
// 2. For each file in each directory, add it to the hashtable if it is not already present, and if it is, check modification time and replace if necessary
// 3. For each subdirectory in each directory, add it to the hashtable if it not already present, and then recurse into the subdirectory
// 4. Use a valid bool for each directory to hold whether it is empty (invalid) or not (valid) and base this value on an OR of its bool with the result of the recursive call
// 5. Set the result of the recursive call to true if files are found in the directory, or recursed subdirectories return true
// 6. Loop through all directories in the hashtable and create them if they don't exist and their valid bool is true
// 7. Loop through all files in the hashtable and sync their contents across all directories
// 8. Celebrate!

int main(int argc, char **argv) {
    struct flags *flags = malloc_data(sizeof(struct flags)); // Allocate memory for the flags struct
    // Set all the flags to their default values
    flags->all_flag = false;
    flags->ignore1 = NULL;
    flags->no_sync_flag = false;
    flags->only1 = NULL;
    flags->copy_perm_time_flag = false;
    flags->recursive_flag = false;
    flags->verbose_flag = false;
    opterr = 0; // Stop getopt from printing error messages
    int opt; // The current option
    while ((opt = getopt(argc, argv, "ai:no:prv")) != -1) {
        // Loop through the options
        switch (opt) {
            case 'a':
                // Set the all flag to true
                flags->all_flag = true;
                break;
            case 'i':
                // Add the pattern to the ignore1 linked list
                enqueue_pattern(&(flags->ignore1), optarg);
                break;
            case 'n':
                // Set the no sync flag and the verbose flag to true
                flags->no_sync_flag = true;
                flags->verbose_flag = true;
                break;
            case 'o':
                // Add the pattern to the only1 linked list
                enqueue_pattern(&(flags->only1), optarg);
                break;
            case 'p':
                // Set the copy permissions and modification time flag to true
                flags->copy_perm_time_flag = true;
                break;
            case 'r':
                // Set the recursive flag to true
                flags->recursive_flag = true;
                break;
            case 'v':
                // Set the verbose flag to true
                flags->verbose_flag = true;
                break;
            case '?':
                // Print an error message and exit the program if an unknown option is passed
                fprintf(stderr, "Unknown option -%c.\n", optopt);
                free_patterns(flags->ignore1);
                free_patterns(flags->only1);
                free(flags);
                return 1;
            default:
                // Print an error message and abort the program if an unknown error occurs
                fprintf(stderr, "Error: unknown error occurred\n");
                free_patterns(flags->ignore1);
                free_patterns(flags->only1);
                free(flags);
                abort();
        }
    }
    int num_directories = argc - optind; // Set the number of directories to the number of command line arguments minus the number of options
    if (num_directories < 2) {
        // Print an error message and exit the program if there are not enough directories
        fprintf(stderr, "Error: not enough directories specified\n");
        free_patterns(flags->ignore1);
        free_patterns(flags->only1);
        free(flags);
        return 1;
    }
    char **directories = malloc_data((num_directories) * sizeof(char *)); // Allocate memory for the array of directory names
    for (int i = 0; i < num_directories; i++) {
        // Loop through the command line arguments and add them to the array of directory names
        if (access(argv[i+optind], F_OK) == -1) {
            // Print an error message and exit the program if the directory does not exist
            fprintf(stderr, "Error: directory %s does not exist\n", argv[i+optind]);
            for (int j = 0; j < i; j++) {
                // Loop through the array of directory names and free the memory allocated for each of them
                free(directories[j]);
            }
            free_patterns(flags->ignore1);
            free_patterns(flags->only1);
            free(flags);
            return 1;
        }
        directories[i] = strdup(argv[i+optind]); // Add the directory name to the array of directory names
    }
    sync_directories(directories, num_directories, flags); // Sync the directories
    for (int i = 0; i < num_directories; i++) {
        // Loop through the array of directory names and free the memory allocated for each of them
        free(directories[i]);
    }
    // Free the memory allocated for the array of directory names, the ignore1 linked list, the only1 linked list, and the flags struct
    free(directories);
    free_patterns(flags->ignore1);
    free_patterns(flags->only1);
    free(flags);
    return 0; // Exit the program
}