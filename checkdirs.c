#include "mysync.h"

bool check_directories(char **directories, int num_directories, struct flags *flags) {
    // A function that takes an array of directory names, and returns true if all the directories exist, and false otherwise
    for (int i = 0; i < num_directories; i++) {
        if (access(directories[i], F_OK) == -1) {
            VERBOSE_PRINT("Directory %s does not exist\n", directories[i]);
            return false;
        }
    }
    VERBOSE_PRINT("All directories exist\n");
    return true;
}