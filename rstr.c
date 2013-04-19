#include "common.h"
#include "rstr.h"
#include "headers.h"

const char ident_name[] = "<name> ";
const int size_ident_name = 7;
const char ident_owner[] = "<owner> ";
const int size_ident_owner = 8;
const char ident_size[] = "<size(bytes)> ";
const int size_ident_size = 14;
const char ident_modified[] = "<last_modified> ";
const int size_ident_modified = 16;

extern char **bckp_directories;

// path of the dource 
extern char *pathS;
extern char *pathD;
extern DIR *dirS;
extern DIR *dirD;

// structures needed to address the files
char **time_folder;
char ***files_on_folder;
char **existing_files;

int main(int argc, char **argv) {
  // sets the umask, to make shure it is possible to create the files as the
  // current user
  //setbuf(stdout, NULL);
  umask(~(S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP |S_IROTH));
  
  /**
   * arguments verification
   */
  
  // if the program was not used correctly:
  if (argc != 3) {
    printf("usage: %s <source-directory> <destination-directory> ",argv[0]); 
    printf("<time-interval-between-backups>\n");
    exit(-1);
  }
  
  // cuts the paths untill the last directory on it, get the absolute paths, and 
  //verify them
  
  pathS = malloc(sizeof(char) * PATH_MAX);
  pathD = malloc(sizeof(char) * PATH_MAX);
  strcpy(pathS, argv[1]);
  strcpy(pathD, argv[2]);
  
  char *tmp = malloc(sizeof(char)*PATH_MAX);
  
  struct stat temp_dir;
  
  if (realpath(pathS, tmp) == NULL) {
    perror("realpath()");
    exit(-1);
  }
  
  
  if (lstat(tmp, &temp_dir) == -1) {
    perror("lstat()");
    exit(-1);
  }
  
  if (!S_ISDIR(temp_dir.st_mode)) {
    
    if ((tmp = dirname(tmp)) == NULL) {
      perror("dirname()");
      exit(-1);
    }
  }
  
  strcpy(pathS, tmp);
  
  // create the destination directory if it not exists
  int dirD_exists = mkdir(pathD, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | 
  S_IROTH);
  
  if (realpath(pathD, tmp) == NULL) {
    perror("realpath()");
    exit(-1);
  }
  
  if (dirD_exists) {
    if (lstat(tmp, &temp_dir) == -1) {
      perror("lstat()");
      exit(-1);
    }
    
    if (!S_ISDIR(temp_dir.st_mode)) {
      if ((tmp = dirname(tmp)) == NULL) {
	perror("dirname()");
	exit(-1);
      }
    }
  }
  
  strcpy(pathD, tmp);
  
  if ((dirS = opendir(pathS)) == NULL) {
    perror("opendir(pathS)");
  }
  
  if ((dirD = opendir(pathD)) == NULL) {
    perror("opendir(pathD)");
  }
  
  /**
   * end of arguments verification
   */
  
  return 0;
}


int fillStructures() {
  return 0;
}


int verifyIfValidFolder(char *pathname) {
  if (pathname == NULL) {
    return 0;
  } else {
    int time[6];
    if (sscanf(pathname, "%4d_%2d_%2d_%2d_%2d_%2d", &time[0], &time[1], &time[2], &time[3], &time[4], &time[5]) == 6) {
      return -1;
    } else {
      return 0;
    }
  }
}