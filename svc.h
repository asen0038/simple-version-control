#ifndef svc_h
#define svc_h

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

struct head{ //(HEAD) pointer pointing to the current branch being used (checked out), initially points to master branch
  struct branch* curr_branch;
  struct commit** all_commits; //This is a malloced list of commits used to simplify the clean up proccess and traversals of commits
  size_t com_count;
};

struct branch{ //malloced, A doubly linked list used to keep track of branhces created
  char* name;
  char** files; //all files ever added sorted alphabetically
  size_t file_count;
  char** track_files; //files in this branch (malloced) After commit a copy is stored of the files
  size_t track_file_count;
  struct commit* current; //pointer to a malloced commit
  struct branch* prev;
  struct branch* next;
};

struct commit{ //malloced, A graph keeping track of commits made
  char* commit_id;
  char* message;
  char* branch_name;
  struct commit** prev; //pointer list to previous (parent) commits made
  size_t n_prev;
  char** files; //tracked files only
  size_t file_count;
  int* hashes; //tracked files hashes in the current state of commit
  size_t hash_count;
  char** contents; //each tracked files content
  size_t n_contents;
  int* bytes_read; //bytes read from tracked files used to restore when reset
  size_t n_bytes;
};

typedef struct resolution {
    // NOTE: DO NOT MODIFY THIS STRUCT
    char *file_name;
    char *resolved_file;
} resolution;

void *svc_init(void);

void cleanup(void *helper);

int hash_file(void *helper, char *file_path);

char *svc_commit(void *helper, char *message);

void *get_commit(void *helper, char *commit_id);

char **get_prev_commits(void *helper, void *commit, int *n_prev);

void print_commit(void *helper, char *commit_id);

int svc_branch(void *helper, char *branch_name);

int svc_checkout(void *helper, char *branch_name);

char **list_branches(void *helper, int *n_branches);

int svc_add(void *helper, char *file_name);

int svc_rm(void *helper, char *file_name);

int svc_reset(void *helper, char *commit_id);

char *svc_merge(void *helper, char *branch_name, resolution *resolutions, int n_resolutions);

#endif
