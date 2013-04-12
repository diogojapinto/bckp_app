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
    printf("usage: %s <source-directory> <destination-directory> <time-interval-between-backups>", argv[0]);
    return -1;
  }
  
  // verifies if the 3rd arg is an integer
  /*char *c;
   * for ( c = argv[3]; c != NULL;) {
   *   if (!isdigit(*c)) {
   *     printf("the 3rd argument must me an integer\n");
   *     return -1;
} else {
  c++;
}
}*/
  
  // cuts the paths untill the last directory on it, get the absolute paths, and verify them
  
  pathS = argv[1];
  pathD = argv[2];
  char *tmp;
  
  struct stat temp_dir;
  
  stat(pathS, &temp_dir);
  tmp = pathS;
  if (!S_ISDIR(temp_dir.st_mode)) {
    
    if ((tmp = dirname(pathS)) == NULL) {
      perror("dirname");
      return -1;
    }
  }
  
  if (realpath(tmp, pathS) == NULL) {
    perror("realpath");
    return -1;
  }
  
  
  stat(pathD, &temp_dir);
  tmp = pathD;
  if (!S_ISDIR(temp_dir.st_mode)) {
    
    if ((tmp = dirname(pathD)) == NULL) {
      perror("dirname");
      return -1;
    }
  }
  
  if (realpath(tmp, pathD) == NULL) {
    perror("realpath");
    return -1;
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
  
  printf("%s\n%s\n",pathS,pathD);
  return 0;
}

void exitHandler(void) {
  free(pathS);
  free(pathD);
  
  closedir(dirS);
  closedir(dirD);
}