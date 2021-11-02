#include "svc.h"

/* For information on the additional data structures implemented
   please look at svc.h
*/

int compare(const void *arg1, const void *arg2){ //comparator funciton for qsort
   return(strcasecmp(*(char **)arg1, *(char **)arg2));
}

void sort(struct branch* b){ //function used for sorting alpahbaetically
  qsort(b->track_files, b->track_file_count, sizeof(char*), compare);
}

void *svc_init(void) {

    struct branch* b = malloc(sizeof(struct branch)); //Initialises the master branch with default field
    b->name = strdup("master");
    b->files = malloc(sizeof(char*));
    b->file_count = 0;
    b->track_files = malloc(sizeof(char*));
    b->track_file_count = 0;
    b->prev = NULL;
    b->next = NULL;
    b->current = NULL;

    struct head* helper = malloc(sizeof(struct head));
    helper->com_count = 0;
    helper->all_commits = malloc(sizeof(struct commit*));
    helper->curr_branch = b;

    return (void*)helper;
}

void cleanup(void *helper) {

    while(((struct head*)helper)->curr_branch->prev != NULL){ //Setting head to master branch (first in list)
      ((struct head*)helper)->curr_branch = ((struct head*)helper)->curr_branch->prev;
    }

    struct branch* temp = ((struct head*)helper)->curr_branch;
    struct branch* next;

    while(temp != NULL){ //frees branches
      next = temp->next;
      for(size_t i = 0; i < temp->file_count; i++){
        free(temp->files[i]);
        temp->files[i] = NULL;
      }
      for(size_t i = 0; i < temp->track_file_count; i++){
        free(temp->track_files[i]);
        temp->track_files[i] = NULL;
      }
      free(temp->files);
      temp->files = NULL;
      free(temp->track_files);
      temp->track_files = NULL;
      free(temp->name);
      temp->name = NULL;
      free(temp);
      temp = next;
    }

    if(((struct head*)helper)->all_commits != NULL){ //free commits
      for(size_t i = 0; i < ((struct head*)helper)->com_count; i++){
        if(((struct head*)helper)->all_commits[i] != NULL){
          free(((struct head*)helper)->all_commits[i]->commit_id);
          ((struct head*)helper)->all_commits[i]->commit_id = NULL;
          free(((struct head*)helper)->all_commits[i]->message);
          ((struct head*)helper)->all_commits[i]->message = NULL;
          free(((struct head*)helper)->all_commits[i]->branch_name);
          ((struct head*)helper)->all_commits[i]->branch_name = NULL;
          free(((struct head*)helper)->all_commits[i]->prev);
          ((struct head*)helper)->all_commits[i]->prev = NULL;
          for(size_t j = 0; j < ((struct head*)helper)->all_commits[i]->file_count; j++){
            free(((struct head*)helper)->all_commits[i]->files[j]);
            ((struct head*)helper)->all_commits[i]->files[j] = NULL;
          }
          free(((struct head*)helper)->all_commits[i]->files);
          ((struct head*)helper)->all_commits[i]->files = NULL;
          for(size_t j = 0; j < ((struct head*)helper)->all_commits[i]->n_contents; j++){
            free(((struct head*)helper)->all_commits[i]->contents[j]);
            ((struct head*)helper)->all_commits[i]->contents[j] = NULL;
          }
          free(((struct head*)helper)->all_commits[i]->contents);
          ((struct head*)helper)->all_commits[i]->contents = NULL;
          free(((struct head*)helper)->all_commits[i]->bytes_read);
          ((struct head*)helper)->all_commits[i]->bytes_read = NULL;
          free(((struct head*)helper)->all_commits[i]->hashes);
          ((struct head*)helper)->all_commits[i]->hashes = NULL;
          free(((struct head*)helper)->all_commits[i]);
          ((struct head*)helper)->all_commits[i] = NULL;
        }
      }
      free(((struct head*)helper)->all_commits);
      ((struct head*)helper)->all_commits = NULL;
    }

    free((struct head*)helper);
    helper = NULL;

    return;
}

int hash_file(void *helper, char *file_path) {

    if(file_path == NULL){
      return -1;
    }

    FILE *f = fopen(file_path, "r+");
    if(f == NULL){
      return -2;
    }

    fseek(f, 0, SEEK_END);
    long int file_length = ftell(f); //reads the number of bytes in the file
    fclose(f);

    f = fopen(file_path, "r+");
    unsigned char* file_contents = (unsigned char*)malloc(file_length);
    int bytes = fread(file_contents, sizeof(unsigned char), file_length, f); //Read file contents and bytes read
    fclose(f);

    int hash = 0;
    for(size_t i = 0; i < strlen(file_path); i++){
      hash = (hash + file_path[i]) % 1000;
    }

    for(size_t i = 0; i < bytes; i++){
      hash = (hash + file_contents[i]) % 2000000000;
    }

    free(file_contents);
    return hash;
}

void set_comm_Id(void* helper, struct branch* commit_branch){ //Calculates commit id in the current branch and checks if any changes made
  if(commit_branch == NULL){
    return;
  }

  int id = 0;
  for(size_t i = 0; i < strlen(commit_branch->current->message); i++){
    id = (id + commit_branch->current->message[i]) % 1000;
  }

  int change_counter = 0;
  sort(commit_branch); //sorts tracked files
  qsort(commit_branch->files, commit_branch->file_count, sizeof(char*), compare);

  if(commit_branch->current->n_prev > 0){

    for(size_t i = 0; i < commit_branch->file_count; i++){ //Look through all the files ever added
      int in_prev_commit = 0;
      int in_curr_bran = 0;

      int x = 0;
      int y = 0;

      for(size_t j = 0; j < commit_branch->current->prev[0]->file_count; j++){ //previous commit
        if(strcmp(commit_branch->files[i], commit_branch->current->prev[0]->files[j]) == 0){
          in_prev_commit = 1;
          x = j;
          break;
        }
      }

      for(size_t j = 0; j < commit_branch->track_file_count; j++){ //current tracking
        if(strcmp(commit_branch->files[i], commit_branch->track_files[j]) == 0){
          FILE* c = fopen(commit_branch->files[i], "r+");
          if(c == NULL){
            in_curr_bran = 0;
            svc_rm(helper, commit_branch->files[i]); //remove from track files
          }else{
            in_curr_bran = 1;
            fclose(c);
          }
          y = j;
          break;
        }
      }

      if(in_prev_commit == 0 && in_curr_bran == 1){ //Add
        id = id + 376591;
        for(size_t j = 0; j < strlen(commit_branch->files[i]); j++){
          id = ((id * (commit_branch->files[i][j] % 37)) % 15485863) + 1;
        }
        change_counter++;
      }

      if(in_prev_commit == 1 && in_curr_bran == 0){ //Deleted
        id = id + 85973;
        for(size_t j = 0; j < strlen(commit_branch->files[i]); j++){
          id = ((id * (commit_branch->files[i][j] % 37)) % 15485863) + 1;
        }
        change_counter++;
      }

      if(in_prev_commit == 1 && in_curr_bran == 1){ //Found and check for modification
        if(commit_branch->current->prev[0]->hashes[x] != hash_file((void*)commit_branch, commit_branch->files[y])){
          id = id + 9573681;
          for(size_t j = 0; j < strlen(commit_branch->files[i]); j++){
            id = ((id * (commit_branch->files[i][j] % 37)) % 15485863) + 1;
          }
          change_counter++;
        }
      }

    }

  }else{
      for(size_t i = 0; i < commit_branch->track_file_count; i++){
        FILE* k = fopen(commit_branch->track_files[i], "r+");
        if(k  == NULL){
          svc_rm(helper, commit_branch->track_files[i]);
          i--;
        }else{
          fclose(k);
          id = id + 376591; //Initial commit files added only, as no previous commit to compare from
          for(size_t j = 0; j < strlen(commit_branch->track_files[i]); j++){
            id = ((id * (commit_branch->track_files[i][j] % 37)) % 15485863) + 1;
          }
          change_counter++;
        }
      }
  }

  for(size_t i = 0; i < commit_branch->track_file_count; i++){
      commit_branch->current->file_count++;
      commit_branch->current->files = realloc(commit_branch->current->files, sizeof(char*) * commit_branch->current->file_count);
      commit_branch->current->files[commit_branch->current->file_count - 1] = strdup(commit_branch->track_files[i]);

      commit_branch->current->n_contents++;
      commit_branch->current->contents = realloc(commit_branch->current->contents, sizeof(char*) * commit_branch->current->n_contents);

      FILE *f = fopen(commit_branch->track_files[i], "r+");
      fseek(f, 0, SEEK_END);
      long int file_length = ftell(f); //reads the number of bytes in the file
      fclose(f);

      f = fopen(commit_branch->track_files[i], "r+");
      commit_branch->current->contents[commit_branch->current->n_contents - 1] = (char*)malloc(file_length);
      int bytes = fread(commit_branch->current->contents[commit_branch->current->n_contents - 1], sizeof(char), file_length, f); //Read file contents and bytes read
      fclose(f);

      commit_branch->current->n_bytes++;
      commit_branch->current->bytes_read = realloc(commit_branch->current->bytes_read, sizeof(int) * commit_branch->current->n_bytes);
      commit_branch->current->bytes_read[commit_branch->current->n_bytes - 1] = bytes;

      commit_branch->current->hash_count++;
      commit_branch->current->hashes = realloc(commit_branch->current->hashes, sizeof(int) * commit_branch->current->hash_count);
      commit_branch->current->hashes[commit_branch->current->hash_count - 1] = hash_file((void*)commit_branch, commit_branch->track_files[i]);

  }

  if(change_counter == 0){
    commit_branch->current->commit_id = NULL;
  }else{
    commit_branch->current->commit_id = malloc(sizeof(char)*7);
    sprintf(commit_branch->current->commit_id, "%06x", id);
    commit_branch->current->branch_name = strdup(commit_branch->name);
  }

}

char *svc_commit(void *helper, char *message) {
    if(message == NULL || ((((struct head*)helper)->curr_branch->file_count == 0) && (((struct head*)helper)->curr_branch->track_file_count == 0))){
      return NULL;//empty svc init repository
    }

    if(((struct head*)helper)->curr_branch->current == NULL){ //first commit
      struct commit* com = malloc(sizeof(struct commit));
      com->message = strdup(message);
      com->n_prev = 0;
      com->file_count = 0;
      com->files = malloc(sizeof(char*));
      com->n_contents = 0;
      com->contents = malloc(sizeof(char*));
      com->bytes_read = malloc(sizeof(int));
      com->n_bytes = 0;
      com->hash_count = 0;
      com->hashes = malloc(sizeof(int));
      ((struct head*)helper)->curr_branch->current = com;
      set_comm_Id(helper, ((struct head*)helper)->curr_branch);

      if(com->commit_id == NULL){
        free(com->message);
        com->message = NULL;
        for(size_t i = 0; i < com->file_count; i++){
          free(com->files[i]);
          com->files[i] = NULL;
        }
        free(com->files);
        com->files = NULL;
        for(size_t i = 0; i < com->n_contents; i++){
          free(com->contents[i]);
          com->contents[i] = NULL;
        }
        free(com->contents);
        com->contents = NULL;
        free(com->bytes_read);
        com->bytes_read = NULL;
        free(com->hashes);
        com->hashes = NULL;
        free(com);
        com = NULL;
        ((struct head*)helper)->curr_branch->current = NULL;
        return NULL;
      }else{
        com->prev = malloc(sizeof(struct commit*));

        ((struct head*)helper)->com_count++;
        ((struct head*)helper)->all_commits = realloc(((struct head*)helper)->all_commits, sizeof(struct commit*) * ((struct head*)helper)->com_count);
        ((struct head*)helper)->all_commits[((struct head*)helper)->com_count - 1] = com;

        return com->commit_id;
      }

    }else{ //other commits after first commits
      struct commit* com = malloc(sizeof(struct commit));
      com->message = strdup(message);
      com->n_prev = 0;
      com->file_count = 0;
      com->files = malloc(sizeof(char*));
      com->n_contents = 0;
      com->contents = malloc(sizeof(char*));
      com->bytes_read = malloc(sizeof(int));
      com->n_bytes = 0;
      com->hash_count = 0;
      com->hashes = malloc(sizeof(int));
      com->n_prev++;
      com->prev = malloc(sizeof(struct commit*) * com->n_prev);
      com->prev[com->n_prev - 1] = ((struct head*)helper)->curr_branch->current;
      ((struct head*)helper)->curr_branch->current = com;
      set_comm_Id(helper, ((struct head*)helper)->curr_branch);

      if(com->commit_id == NULL){
        ((struct head*)helper)->curr_branch->current = com->prev[0];
        free(com->prev);
        free(com->message);
        com->message = NULL;
        for(size_t i = 0; i < com->file_count; i++){
          free(com->files[i]);
          com->files[i] = NULL;
        }
        free(com->files);
        com->files = NULL;
        for(size_t i = 0; i < com->n_contents; i++){
          free(com->contents[i]);
          com->contents[i] = NULL;
        }
        free(com->contents);
        com->contents = NULL;
        free(com->bytes_read);
        com->bytes_read = NULL;
        free(com->hashes);
        com->hashes = NULL;
        free(com);
        com = NULL;

        return NULL;
      }else{

        ((struct head*)helper)->com_count++;
        ((struct head*)helper)->all_commits = realloc(((struct head*)helper)->all_commits, sizeof(struct commit*) * ((struct head*)helper)->com_count);
        ((struct head*)helper)->all_commits[((struct head*)helper)->com_count - 1] = com;

        return com->commit_id;
      }
    }

    return NULL;
}

void *get_commit(void *helper, char *commit_id) {

    if(commit_id == NULL){
      return NULL;
    }

    for(size_t i = 0; i < ((struct head*)helper)->com_count; i++){
      if(strcmp(commit_id, ((struct head*)helper)->all_commits[i]->commit_id) == 0){
        return (void*)(((struct head*)helper)->all_commits[i]);
      }
    }

    return NULL;
}

char **get_prev_commits(void *helper, void *commit, int *n_prev) {

    if(commit == NULL || n_prev == NULL){
      *n_prev = 0;
      return NULL;
    }

    *n_prev = 0;
    char** ids = NULL;
    for(size_t i = 0; i < ((struct commit*)commit)->n_prev; i++){
      *n_prev += 1;
      ids = realloc(ids, sizeof(char*) * (*n_prev));
      ids[*(n_prev) - 1] = ((struct commit*)commit)->prev[i]->commit_id;
    }

    if(*n_prev == 0){
      return NULL;
    }

    return ids;
}

void print_commit(void *helper, char *commit_id) {
    if(commit_id == NULL){
      puts("Invalid commit id");
      return;
    }

    int found = 0;
    struct commit* c = NULL;
    for(size_t i = 0; i < ((struct head*)helper)->com_count; i++){
      if(strcmp(((struct head*)helper)->all_commits[i]->commit_id, commit_id) == 0){
        c = ((struct head*)helper)->all_commits[i];
        found = 1;
        break;
      }
    }

    if(found == 0){
      puts("Invalid commit id");
      return;
    }

    struct branch* br = ((struct head*)helper)->curr_branch;
    while(br->prev != NULL){
      br = br->prev;
    }

    struct branch* b = NULL;
    while(br != NULL){ //Check all branches
      if(strcmp(br->name, c->branch_name) == 0){
        b = br;
        break;
      }
      br = br->next;
    }


    printf("%s [%s]: %s\n", c->commit_id, b->name ,c->message);

    if(c->n_prev > 0){ //All other commits
      for(size_t i = 0; i < b->file_count; i++){ //Look through all the files ever added
        int in_prev_commit = 0;
        int in_curr_com = 0;

        int x = 0;
        int y = 0;

        for(size_t j = 0; j < c->prev[0]->file_count; j++){ //previous commit
          if(strcmp(b->files[i], c->prev[0]->files[j]) == 0){
            in_prev_commit = 1;
            x = j;
            break;
          }
        }

        for(size_t j = 0; j < c->file_count; j++){ //current commit
          if(strcmp(b->files[i], c->files[j]) == 0){
            in_curr_com = 1;
            y = j;
            break;
          }
        }

        if(in_prev_commit == 0 && in_curr_com == 1){ //Add
          printf("%5s %s\n", "+", b->files[i]);
        }

        if(in_prev_commit == 1 && in_curr_com == 0){ //Deleted
          printf("%5s %s\n", "-", b->files[i]);
        }

        if(in_prev_commit == 1 && in_curr_com == 1){ //Found and check for modification
          if(c->prev[0]->hashes[x] != c->hashes[y]){
            printf("%5s %s [%d --> %d]\n", "/", b->files[i], c->prev[0]->hashes[x], c->hashes[y]);
          }
        }

      }

      printf("\n");
      printf("%17s (%zu):\n", "Tracked files", c->file_count);
      for(size_t i = 0; i < c->file_count; i++){
        printf("%4s[%10d] %s\n", "", c->hashes[i], c->files[i]);
      }

      return;
    }else{ //Initial commit (add only)
      for(size_t i = 0; i < c->file_count; i++){
        printf("%5s %s\n", "+", c->files[i]);
      }
      printf("\n");

      printf("%17s (%zu):\n", "Tracked files", c->file_count);
      for(size_t i = 0; i < c->file_count; i++){
        printf("%4s[%10d] %s\n", "", c->hashes[i], c->files[i]);
      }

      return;
    }

}

int svc_branch(void *helper, char *branch_name) {

    if(branch_name == NULL){
      return -1;
    }

    int is_valid = 0;
    for(size_t i = 0; i < strlen(branch_name); i++){ //Validity check
      if(branch_name[i] == 45){
        is_valid++;
      }else if(branch_name[i] >= 47 && branch_name[i] <= 57){
        is_valid++;
      }else if(branch_name[i] >= 65 && branch_name[i] <= 90){
        is_valid++;
      }else if(branch_name[i] == 95){
        is_valid++;
      }else if(branch_name[i] >= 97 && branch_name[i] <= 122){
        is_valid++;
      }
    }

    if(is_valid < strlen(branch_name)){
      return -1;
    }

    struct branch* br = ((struct head*)helper)->curr_branch;
    while(br->prev != NULL){
      br = br->prev;
    }

    while(br != NULL){ //Check if branch name already used
      if(strcmp(br->name, branch_name) == 0){
        return -2;
      }
      br = br->next;
    }

    for(size_t i = 0; i < ((struct head*)helper)->curr_branch->file_count; i++){ //Check for uncomitted changes
      int in_track = 0;
      int in_curr_com = 0;

      int x = 0;
      int y = 0;

      for(size_t j = 0; j < ((struct head*)helper)->curr_branch->current->file_count; j++){ //in commit
        if(strcmp(((struct head*)helper)->curr_branch->files[i], ((struct head*)helper)->curr_branch->current->files[j]) == 0){
          in_curr_com = 1;
          x = j;
          break;
        }
      }

      for(size_t j = 0; j < ((struct head*)helper)->curr_branch->track_file_count; j++){ //in track
        if(strcmp(((struct head*)helper)->curr_branch->files[i], ((struct head*)helper)->curr_branch->track_files[j]) == 0){
          in_track = 1;
          y = j;
          break;
        }
      }

      if(in_curr_com == 0 && in_track == 1){ //Add
        return -3;
      }

      if(in_curr_com == 1 && in_track == 0){ //Deleted
        return -3;
      }

      if(in_curr_com == 1 && in_track == 1){ //Found and check for modification
        if(((struct head*)helper)->curr_branch->current->hashes[x] != hash_file(helper, ((struct head*)helper)->curr_branch->track_files[y])){
          return -3;
        }
      }
    }

    //Now the new branch can be crearted
    struct branch* b = malloc(sizeof(struct branch));
    b->name = strdup(branch_name);
    b->files = malloc(sizeof(char*));
    b->file_count = 0;
    b->track_files = malloc(sizeof(char*));
    b->track_file_count = 0;

    struct branch* cursor = ((struct head*)helper)->curr_branch;
    while(cursor->next != NULL){
      cursor = cursor->next;
    }

    b->prev = cursor;
    b->next = NULL;
    cursor->next = b;
    b->current = ((struct head*)helper)->curr_branch->current;

    //copy all to the branch
    for(size_t i = 0; i < ((struct head*)helper)->curr_branch->file_count; i++){
      b->file_count++;
      b->files = realloc(b->files, sizeof(char*) * b->file_count);
      b->files[b->file_count - 1] = strdup(((struct head*)helper)->curr_branch->files[i]);
    }

    for(size_t i = 0; i < ((struct head*)helper)->curr_branch->track_file_count; i++){
      b->track_file_count++;
      b->track_files = realloc(b->track_files, sizeof(char*) * b->track_file_count);
      b->track_files[b->track_file_count - 1] = strdup(((struct head*)helper)->curr_branch->track_files[i]);
    }

    return 0;
}

int svc_checkout(void *helper, char *branch_name) {

    if(branch_name == NULL){
      return -1;
    }

    struct branch* br = ((struct head*)helper)->curr_branch;
    while(br->prev != NULL){
      br = br->prev;
    }

    int found = 0;
    struct branch* b;
    while(br != NULL){ //Check if exists
      if(strcmp(br->name, branch_name) == 0){
        b = br;
        found = 1;
        break;
      }
      br = br->next;
    }

    if(found == 0){
      return -1;
    }

    for(size_t i = 0; i < ((struct head*)helper)->curr_branch->file_count; i++){ //Check for uncomitted changes
      int in_track = 0;
      int in_curr_com = 0;

      int x = 0;
      int y = 0;

      for(size_t j = 0; j < ((struct head*)helper)->curr_branch->current->file_count; j++){ //in commit
        if(strcmp(((struct head*)helper)->curr_branch->files[i], ((struct head*)helper)->curr_branch->current->files[j]) == 0){
          in_curr_com = 1;
          x = j;
          break;
        }
      }

      for(size_t j = 0; j < ((struct head*)helper)->curr_branch->track_file_count; j++){ //in track
        if(strcmp(((struct head*)helper)->curr_branch->files[i], ((struct head*)helper)->curr_branch->track_files[j]) == 0){
          in_track = 1;
          y = j;
          break;
        }
      }

      if(in_curr_com == 0 && in_track == 1){ //Add
        return -2;
      }

      if(in_curr_com == 1 && in_track == 0){ //Deleted
        return -2;
      }

      if(in_curr_com == 1 && in_track == 1){ //Found and check for modification
        if(((struct head*)helper)->curr_branch->current->hashes[x] != hash_file(helper, ((struct head*)helper)->curr_branch->track_files[y])){
          return -2;
        }
      }
    }

    FILE* f;
    for(size_t i = 0; i < b->track_file_count; i++){ //restore tracked file contents in the workspace
      f = fopen(b->track_files[i], "w+");
      fwrite(b->current->contents[i], sizeof(char), b->current->bytes_read[i], f);
      fclose(f);

    }

    ((struct head*)helper)->curr_branch = b;

    return 0;
}

char **list_branches(void *helper, int *n_branches) {

    if(n_branches == NULL){
      return NULL;
    }

    (*n_branches) = 0;

    struct branch* cursor = ((struct head*)helper)->curr_branch;
    while(cursor->prev != NULL){//Start from master branch
      cursor = ((struct head*)helper)->curr_branch->prev;
    }

    char** branches = malloc(sizeof(char*));
    while(cursor != NULL){
      (*n_branches) += 1;
      branches = realloc(branches, sizeof(char*) * (*n_branches));
      branches[*(n_branches) - 1] = cursor->name;
      printf("%s\n", cursor->name);
      cursor = cursor->next;
    }

    return branches;
}

int svc_add(void *helper, char *file_name) {

    if(file_name == NULL){
      return -1;
    }

    for(size_t i = 0; i < ((struct head*)helper)->curr_branch->track_file_count; i++){ //Already tracked
      if(strcmp(file_name, ((struct head*)helper)->curr_branch->track_files[i]) == 0){
        return -2;
      }
    }

    FILE *f = fopen(file_name, "r+");
    if(f == NULL){
      return -3;
    }
    fclose(f);

    //Check main list, if already addded don't add again
    int found = 0;
    for(size_t i = 0; i < ((struct head*)helper)->curr_branch->file_count; i++){
      if(strcmp(((struct head*)helper)->curr_branch->files[i], file_name) == 0){
        found = 1;
        break;
      }
    }

    //All files ever added for detecting change
    if(found == 0){
      ((struct head*)helper)->curr_branch->file_count++;
      ((struct head*)helper)->curr_branch->files = realloc(((struct head*)helper)->curr_branch->files, sizeof(char*) * ((struct head*)helper)->curr_branch->file_count);
      ((struct head*)helper)->curr_branch->files[((struct head*)helper)->curr_branch->file_count - 1] = strdup(file_name);
    }

    //Tracked files in staging proccess
    ((struct head*)helper)->curr_branch->track_file_count++;
    ((struct head*)helper)->curr_branch->track_files = realloc(((struct head*)helper)->curr_branch->track_files, sizeof(char*) * ((struct head*)helper)->curr_branch->track_file_count);
    ((struct head*)helper)->curr_branch->track_files[((struct head*)helper)->curr_branch->track_file_count - 1] = strdup(file_name);

    int hash = hash_file(helper, file_name);

    return hash;
}

int svc_rm(void *helper, char *file_name) {

    if(file_name == NULL){
      return -1;
    }

    char copy[60];
    strcpy(copy, file_name);

    int found = 0;
    for(size_t i = 0; i < ((struct head*)helper)->curr_branch->track_file_count; i++){
      if(strcmp(file_name, ((struct head*)helper)->curr_branch->track_files[i]) == 0){ //found in tracked files

        for(size_t j = i; j < ((struct head*)helper)->curr_branch->track_file_count; j++){ //remove from tracked files and resize it
          if(j == ((struct head*)helper)->curr_branch->track_file_count - 1){
            free(((struct head*)helper)->curr_branch->track_files[j]);
            ((struct head*)helper)->curr_branch->track_files[j] = NULL;
            ((struct head*)helper)->curr_branch->track_file_count--;
            ((struct head*)helper)->curr_branch->track_files = realloc(((struct head*)helper)->curr_branch->track_files, sizeof(char*) * ((struct head*)helper)->curr_branch->track_file_count);
            break;
          }else{
            free(((struct head*)helper)->curr_branch->track_files[j]);
            ((struct head*)helper)->curr_branch->track_files[j] = strdup(((struct head*)helper)->curr_branch->track_files[j+1]);
          }
        }

        found = 1;
        return hash_file(helper, copy);
      }
    }

    if(found == 0){
      return -2;
    }

    return 0;
}

int svc_reset(void *helper, char *commit_id) {

    if(commit_id == NULL){
      return -1;
    }

    //Check if commit exists
    int found = 0;
    struct commit* res;
    for(size_t i = 0; i < ((struct head*)helper)->com_count; i++){
      if(strcmp(((struct head*)helper)->all_commits[i]->commit_id, commit_id) == 0){
        res = ((struct head*)helper)->all_commits[i];
        found = 1;
        break;
      }
    }

    if(found == 0){
      return -2;
    }

    //perform reset
    ((struct head*)helper)->curr_branch->current = res;

    for(size_t i = 0; i < ((struct head*)helper)->curr_branch->track_file_count; i++) {
      free(((struct head*)helper)->curr_branch->track_files[i]);
      ((struct head*)helper)->curr_branch->track_files[i] = NULL;
    }
    free(((struct head*)helper)->curr_branch->track_files);
    ((struct head*)helper)->curr_branch->track_files = NULL;

    ((struct head*)helper)->curr_branch->track_file_count = 0;
    ((struct head*)helper)->curr_branch->track_files = malloc(sizeof(char*));

    FILE* f;
    for(size_t i = 0; i < res->file_count; i++){ //restore tracked file contents
      f = fopen(res->files[i], "w+");
      fwrite(res->contents[i], sizeof(char), res->bytes_read[i], f);
      fclose(f);

      ((struct head*)helper)->curr_branch->track_file_count++;
      ((struct head*)helper)->curr_branch->track_files = realloc(((struct head*)helper)->curr_branch->track_files, sizeof(char*) * ((struct head*)helper)->curr_branch->track_file_count);
      ((struct head*)helper)->curr_branch->track_files[((struct head*)helper)->curr_branch->track_file_count - 1] = strdup(res->files[i]);
    }


    return 0;
}

char *svc_merge(void *helper, char *branch_name, struct resolution *resolutions, int n_resolutions) {

    if(branch_name == NULL){
      puts("Invalid branch name");
      return NULL;
    }

    struct branch* br = ((struct head*)helper)->curr_branch;
    while(br->prev != NULL){
      br = br->prev;
    }

    int found = 0;
    struct branch* b; //branch to merge with current
    while(br != NULL){ //Check if exists
      if(strcmp(br->name, branch_name) == 0){
        b = br;
        found = 1;
        break;
      }
      br = br->next;
    }

    if(found == 0){
      puts("Branch not found");
      return NULL;
    }

    if(b == ((struct head*)helper)->curr_branch){
      puts("Cannot merge a branch with itself");
      return NULL;
    }

    for(size_t i = 0; i < ((struct head*)helper)->curr_branch->file_count; i++){ //Check for uncomitted changes
      int in_track = 0;
      int in_curr_com = 0;

      int x = 0;
      int y = 0;

      for(size_t j = 0; j < ((struct head*)helper)->curr_branch->current->file_count; j++){ //in commit
        if(strcmp(((struct head*)helper)->curr_branch->files[i], ((struct head*)helper)->curr_branch->current->files[j]) == 0){
          in_curr_com = 1;
          x = j;
          break;
        }
      }

      for(size_t j = 0; j < ((struct head*)helper)->curr_branch->track_file_count; j++){ //in track
        if(strcmp(((struct head*)helper)->curr_branch->files[i], ((struct head*)helper)->curr_branch->track_files[j]) == 0){
          in_track = 1;
          y = j;
          break;
        }
      }

      if(in_curr_com == 0 && in_track == 1){ //Add
        puts("Changes must be committed");
        return NULL;
      }

      if(in_curr_com == 1 && in_track == 0){ //Deleted
        puts("Changes must be committed");
        return NULL;
      }

      if(in_curr_com == 1 && in_track == 1){ //Found and check for modification
        if(((struct head*)helper)->curr_branch->current->hashes[x] != hash_file(helper, ((struct head*)helper)->curr_branch->track_files[y])){
          puts("Changes must be committed");
          return NULL;
        }
      }
    }

    //Begin merge process
    for(size_t i = 0; i < b->track_file_count; i++){
      svc_add(helper, b->track_files[i]);
    }

    FILE* f;
    char* input;
    for(int i = 0; i < n_resolutions; i++){  //resolve the contents of the file with resolved file
      f = fopen(resolutions[i].resolved_file, "r+");
      if(f == NULL){
        svc_rm(helper, resolutions[i].file_name);
      }else{
        fseek(f, 0, SEEK_END);
        long int s = ftell(f);
        fclose(f);

        f = fopen(resolutions[i].resolved_file, "r+");
        input = (char*)malloc(s);
        fread(input, sizeof(char), s, f);
        fclose(f);

        f = fopen(resolutions[i].file_name, "w+");
        fwrite(input, sizeof(char), s, f);
        fclose(f);
        free(input);
        input = NULL;
      }
    }

    char message[67];
    sprintf(message, "Merged branch %s", branch_name);

    svc_commit(helper, message);
    ((struct head*)helper)->curr_branch->current->n_prev++;
    ((struct head*)helper)->curr_branch->current->prev = realloc(((struct head*)helper)->curr_branch->current->prev, sizeof(struct commit*) * ((struct head*)helper)->curr_branch->current->n_prev);
    ((struct head*)helper)->curr_branch->current->prev[((struct head*)helper)->curr_branch->current->n_prev - 1] = b->current;

    puts("Merge successful");

    return ((struct head*)helper)->curr_branch->current->commit_id;
}
