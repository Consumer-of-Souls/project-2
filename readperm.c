#include "mysync.h"

char *permissions(int permissions) {
    // A function that takes a file's permissions and returns a string representation of them in the form "-rwxrwxrwx"
    char *mode_string = malloc_data(10);
    int mask = 256; // The highest bit of the permissions
    for (int i = 0; i < 9; i++) {
        // Check each bit of the permissions and assign the corresponding character to the mode string
        if (permissions & mask) {
            // The bit is set, so use r, w, or x depending on the position
            mode_string[i] = i % 3 == 0 ? 'r' : (i % 3 == 1 ? 'w' : 'x');
        } else {
            // The bit is not set, so use -
            mode_string[i] = '-';
        }
        mask >>= 1; // Shift the mask to the right by one bit
    }
    mode_string[9] = '\0'; // Terminate the string with a null character
    return mode_string;
}