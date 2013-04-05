#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h> 

#define MAXBUFFER 50 

int main(int argc, char *argv[]) {
  
  struct stat fileToCopy;
  struct dirent fileOnFolder;
  char *path;
  
  path = argv[1];
  
}