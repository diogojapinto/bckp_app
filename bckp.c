#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdlib.h>
#include <libgen.h>
#include <dirent.h>
#include <ctype.h>

#define MAXBUFFER 500


struct stat fileToCopy;
struct dirent fileOnFolder;
char *pathS;
char *pathD;
DIR *dirS;
DIR *dirD;

int main(int argc, char *argv[]) {
  
  
  // if the program was not used correctly:
  if (argc != 4)
  {
    printf("usage: %s <source-directory> <destination-directory> <time-interval-between-backups>\n", argv[0]);
    return -1;
  }
  
  // verifies if the 3rd arg is an integer
  char *c;
  for ( c = argv[3]; c != NULL;) {
    if (!isdigit(*c)) {
      printf("the 3rd argument must me an integer\n");
      return -1;
    } else {
      c++;
    }
  }
  
  // cuts the paths untill the last directory on it, get the absolute paths, and verify them
  
  pathS = argv[1];
  pathD = argv[2];
  char *tmp = malloc(sizeof(char)*PATH_MAX);
  
  struct stat temp_dir;
  
  if (realpath(pathS, tmp) == NULL) {
    perror("realpath");
    return -1;
  }
  
  
  if (stat(tmp, &temp_dir) == -1) {
    perror("statS");
    return -1;
  }
  
  if (!S_ISDIR(temp_dir.st_mode)) {
    
    if ((tmp = dirname(tmp)) == NULL) {
      perror("dirname");
      return -1;
    }
  }
  
  pathS = tmp;
  
  int dirD_exists = mkdir("README.md",S_IRUSR | S_IWUSR | S_IXUSR | S_IROTH);
  
  mkdir("README.md",S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IROTH);
  
  if (realpath(pathD, tmp) == NULL) {
    perror("realpath");
    return -1;
  }
  
  if (stat(tmp, &temp_dir) == -1) {
    perror("statD");
    return -1;
  }
  
  if (!S_ISDIR(temp_dir.st_mode)) {
    if ((tmp = dirname(tmp)) == NULL) {
      perror("dirname");
      return -1;
    }
  }
  
  pathD = tmp;
  
  if (dirD_exists == 0) {
    printf("created %s\n", pathD);
  }
  
  // open the source and destination directories
  if ((dirS = opendir(pathS)) == NULL) {
    perror("opendir(dirS)");
    return -1;
  }
  
  if ((dirD = opendir(pathD)) == NULL) {
    perror("opendir(dirD)");
    return -1;
  }
  
  return 0;
}

void exitHandler(void) {
  free(pathS);
  free(pathD);
  
  closedir(dirS);
  closedir(dirD);
}