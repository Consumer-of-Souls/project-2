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

#ifndef _SC_PAGESIZE
#define _SC_PAGESIZE 4096
#endif

#define DEFAULT_HASHTABLE_SIZE 100

//  you may need other standard header files


//  CITS2002 Project 2 2023
//  Student1:   23751337   JIA QI LAM
//  Student2:   23970936   JACOB READ


//  mysync (v1.0)


struct hashtable {
    // A struct that represents a hashtable
    int size; // The size of the hashtable
    int num_elements; // The number of elements in the hashtable
    struct node **table; // The table of file nodes
};

struct file_names {
    char *name;
    struct file_names *next;
};

struct node {
    char *name;
    void *data;
    struct node *next;
};

struct file {
    int type_id;
    int permissions;
    long long int edit_time;
    long long int size;
    int directory_index;
};

struct dir_indexes {
    int type_id;
    struct index *head;
    struct index *tail; 
};

struct index {
    int index;
    struct index *next;
};

struct pattern {
    regex_t regex;
    struct pattern *next;
};

struct flags {
    int all_flag;
    struct pattern *ignore1;
    int no_sync_flag;
    struct pattern *only1;
    int copy_perm_time_flag;
    int recursive_flag;
    int verbose_flag;
};


#define VERBOSE_PRINT(fmt, ...) \
    do { \
        if (flags->verbose_flag) { \
            printf(fmt, ##__VA_ARGS__); \
        } \
    } while (0)

char *glob2regex(char *);

char *permissions(int);

void sync_master(struct file *, char*, char **, int, struct flags *);

void enqueue_pattern(struct pattern **, char *);

int check_patterns(struct pattern *, char *);

void *malloc_data(size_t);

void sync_directories(char **, int, struct flags *);

void placeholder_dirs(struct dir_indexes *, char *, char **, int, struct flags *);

int check_directories(char **, int, struct flags *);

void free_flags(struct flags *);

void free_file(struct file *);

void put(struct hashtable **, char *, void *);

struct node *get(struct hashtable *, char *);

void delete(struct hashtable **, char *);

struct hashtable *create_hashtable(size_t);

void print_all(struct hashtable *, struct file_names *, char **);

#endif