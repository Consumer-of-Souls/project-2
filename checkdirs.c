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