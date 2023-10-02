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

#ifndef _SC_PAGESIZE
#define _SC_PAGESIZE 4096
#endif

//  you may need other standard header files


//  CITS2002 Project 2 2023
//  Student1:   23751337   JIA QI LAM
//  Student2:   23970936   JACOB READ


//  mysync (v1.0)

struct file_node {
    struct file *file;
    struct file_node *next;
};

struct file {
    char *name;
    int permissions;
    long long int edit_time;
    char *type;
    long long int size;
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

char *glob2regex(char *);

#endif