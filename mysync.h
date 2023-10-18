#ifndef MYSYNC_H
#define MYSYNC_H

#define _POSIX_C_SOURCE     200809L
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>
#include <utime.h>
#include <stdbool.h>
#include <fcntl.h>

#ifndef _SC_PAGESIZE
// If _SC_PAGESIZE is not defined, define it as 4096
#define _SC_PAGESIZE 4096
#endif

#define DEFAULT_HASHTABLE_SIZE 100


//  CITS2002 Project 2 2023
//  Student1:   23751337   JIA QI LAM
//  Student2:   23970936   JACOB READ


//  mysync (v2.0)


struct hashtable {
    // A struct that represents a hashtable
    int size; // The size of the hashtable
    int num_elements; // The number of elements in the hashtable
    struct node **table; // The table of file nodes
};

struct relpaths {
    // A struct that represents a linked list of relative paths
    char *relpath; // The relative path
    struct relpaths *next; // The next relative path
};

struct node {
    // A struct that represents a node in the hashtable
    char *name; // The name of the node (the key)
    void *data; // The data of the node (generic)
    struct node *next; // The next node in the linked list
};

struct file {
    // A struct that represents a file
    int type_id; // An int used when casting to check whether the struct is a file or a dir_indexes type (should always be 1)
    int permissions; // The permissions of the file
    long long int edit_time; // The edit time of the file
    long long int size; // The size of the file
    int directory_index; // The index of the directory that the file is in
};

struct dir_indexes {
    // A struct that represents a directory
    int type_id; // An int used when casting to check whether the struct is a file or a dir_indexes type (should always be 0)
    bool valid; // A bool that represents whether the directory is empty or not
    struct index *head; // The head of the linked list of indexes
    struct index *tail; // The tail of the linked list of indexes
};

struct index {
    // A struct that represents an index in a linked list
    int index; // The index of the directory
    struct index *next; // The next index in the linked list
};

struct pattern {
    // A struct that represents a pattern in a linked list
    regex_t regex; // The regex of the pattern
    struct pattern *next; // The next pattern in the linked list
};

struct flags {
    // A struct that represents the flags passed in the command line arguments
    bool all_flag; // A bool that represents whether the -a flag was passed
    struct pattern *ignore1; // A linked list of patterns that represent the -i flag
    bool no_sync_flag; // A bool that represents whether the -n flag was passed
    struct pattern *only1; // A linked list of patterns that represent the -o flag
    bool copy_perm_time_flag; // A bool that represents whether the -p flag was passed
    bool recursive_flag; // A bool that represents whether the -r flag was passed
    bool verbose_flag; // A bool that represents whether the -v flag was passed
};

// Macros

#define VERBOSE_PRINT(fmt, ...) \
    do { \
        if (flags->verbose_flag) { \
            printf(fmt, ##__VA_ARGS__); \
        } \
    } while (0) 


// Function prototypes

char *glob2regex(char *);

char *permissions(int);

void sync_master(struct file *, char*, char **, int, struct flags *);

void enqueue_pattern(struct pattern **, char *);

bool check_patterns(struct pattern *, char *);

void *malloc_data(size_t);

void sync_directories(char **, int, struct flags *);

void create_directories(struct dir_indexes *, char *, char **, int, struct flags *);

void free_flags(struct flags *);

void put(struct hashtable **, char *, void *);

void *get(struct hashtable *, char *);

void delete(struct hashtable **, char *);

struct hashtable *create_hashtable(size_t);

void print_all(struct hashtable *, struct relpaths *, char **);

void free_patterns(struct pattern *);

#endif