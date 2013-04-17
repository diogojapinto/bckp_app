/**
 * TO ADD:
 * -> the sleep is done in a process, and copy by other... verify if copy is done (handler for sigchld)
 * don't forget the return value of sleep (if != 0, do sleep/ALARM of remaining time)
 */

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdlib.h>
#include <libgen.h>
#include <dirent.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <pwd.h>

/**
 * macros
 */
#define MAXBUFFER 1000
#define TIME_LENGTH 500
#define NR_TIME_ELEMS 20
#define _GNU_SOURCE

struct bckp_direct {
  int last_elem_pos;
  char **directories;
};

char *pathS;
char *pathD;
DIR *dirS;
DIR *dirD;

char ident_name[] = "<name> ";
int size_ident_name = 7;
char ident_owner[] = "<owner> ";
int size_ident_owner = 8;
char ident_size[] = "<size(bytes> ";
int size_ident_size = 13;
char ident_modified[] = "<last_modified> ";
int size_ident_modified = 15;

char* createDestFolder();
int isFileModified(const char *path_s, const char *path_d);
int copyFiles(const char *path_s, const char *path_d);
int fullBackup(char *dest);
int incrementalBackup(char * dest);
int updateBackupInfo(const char *dest, const char *name, const struct stat *st);


int main(int argc, char *argv[]) {
  
  // sets the umask, to make shure it is possible to create the files as the
  // current user
  //setbuf(stdout, NULL);
  umask(~(S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP |S_IROTH));
  
  /**
   * arguments verification
   */
  
  // if the program was not used correctly:
  if (argc != 4)
  {
    printf("usage: %s <source-directory> <destination-directory> <time-interval-between-backups>\n", argv[0]);
    exit(-1);
  }
  
  // verifies if the 3rd arg is an integer
  char *c;
  for ( c = argv[3]; c != NULL && *c != '\0'; c++) {
    if (!isdigit(*c)) {
      printf("the 3rd argument must me an integer\n");
      exit(-1);
    }
  }
  
  //int time_frame = atoi(argv[3]);
  
  // cuts the paths untill the last directory on it, get the absolute paths, and verify them
  
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
  
  
  if (stat(tmp, &temp_dir) == -1) {
    perror("stat()");
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
  int dirD_exists = mkdir(pathD, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IROTH);
  
  if (realpath(pathD, tmp) == NULL) {
    perror("realpath()");
    exit(-1);
  }
  
  if (dirD_exists) {
    if (stat(tmp, &temp_dir) == -1) {
      perror("stat()");
      exit(-1);
    }
    
    if (!S_ISDIR(temp_dir.st_mode)) {
      if ((tmp = dirname(tmp)) == NULL) {
	perror("dirname()");
	exit(-1);
      }
    }
  }
  
  if (dirD_exists == 0) {
    printf("created folder %s\n", pathD);
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
  
  //while(true) {
  char* bckp_dest = createDestFolder();
  fullBackup(bckp_dest);
  
  //  sleep(time_frame);
  //}
  
  return 0;
}

/**
 * creates the destination folder for the backup, with the the name corresponding to
 * the local time
 */
char* createDestFolder() {
  struct tm *local_time;
  time_t current_time;
  if (time(&current_time) == -1) {
    perror("time()");
    exit(-1);
  }
  
  if ((local_time = localtime(&current_time)) == NULL) {
    perror("localtime()");
    exit(-1);
  }
  
  char dir_name[TIME_LENGTH];
  
  if (sprintf(dir_name, "%d_%d_%d_%d_%d_%d", local_time->tm_year + 1900, local_time->tm_mon + 1,
    local_time->tm_mday, local_time->tm_hour, local_time->tm_min, local_time->tm_sec ) >= NR_TIME_ELEMS) {
    write(STDERR_FILENO, "error in naming the folder\n", 27);
  exit(-1);
    }
    
    // concatenate the dir_name with the destination folder
    char *tmp_dir = NULL;
    
    if (tmp_dir == NULL) {
      tmp_dir = malloc(sizeof(char) * PATH_MAX);
    } else {
      free(tmp_dir);
      tmp_dir = malloc(sizeof(char) * PATH_MAX);
    }
    
    strcpy(tmp_dir, pathD);
    strcat(tmp_dir, "/");
    strcat(tmp_dir, dir_name);
    
    if (mkdir(tmp_dir, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IROTH)) {
      perror("mkdir()");
      exit(-1);
    }
    
    return tmp_dir;
}

/**
 * return 1 if files are different, 0 otherwise
 * 
 * (could have used the field st_mtime from the stat structure correspondent to the source files
 * but there is the possibility of a file being modified in a time interval lower than one second)
 */
int isFileModified(const char *path_s, const char *path_d) {
  int source, destination;
  char cs, cd;
  
  // tries to open the files
  if ((source =open(path_s, O_RDONLY) ) == -1) {
    perror("open()");
    exit(-1);
  }
  
  if ((destination = open(path_d, O_RDONLY)) == -1) {
    perror("open()");
    exit(-1);
  }
  
  do {
    // reads a char from the files
    if (read(source, &cs, 1) != read(destination, &cd, 1)) {
      close(source);
      close(destination);
      return 1;
    } else {
      if (cs != cd) {
	close(source);
	close(destination);
	return 1;
      }
    }
    
  } while ( cs != '\0' && cs != '\0');
  
  // closes the files compared
  close(source);
  close(destination);
  return 0;
}

int copyFiles(const char *path_s, const char *path_d) {
  int source, destination;
  char c;
  
  // tries to open the files
  if ((source = open(path_s, O_RDONLY) ) == -1) {
    perror("open()");
    exit(-1);
  }
  
  if ((destination = open(path_d, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IROTH)) == -1) {
    perror("open()");
    exit(-1);
  }
  
  read(source, &c, 1);
  do {
    if (write(destination, &c, 1) == 0) {
      perror("write()");
      exit(-1);
    }
  } while (read(source, &c, 1));
  
  // closes the files compared
  close(source);
  close(destination);
  return 0;
}

int fullBackup(char * dest) {
  struct dirent *src = NULL;
  while((src = readdir(dirS)) != NULL) {
    printf("\n%s\n", src->d_name);
    struct stat st_src;
    
    // prepares the pathnames for source file
    char tmp_s[PATH_MAX];
    strcpy(tmp_s, pathS);
    strcat(tmp_s,"/");
    strcat(tmp_s,src->d_name);
    
    // verifies if the source file is regular
    if (stat(tmp_s, &st_src) == -1) {
      perror("stat()");
      exit(-1);
    }
    if (!S_ISREG(st_src.st_mode)) {
      continue;
    }
    
    // prepares the pathnames for source file
    char tmp_d[PATH_MAX];
    strcpy(tmp_d, dest);
    strcat(tmp_d,"/");
    strcat(tmp_d,src->d_name);
    copyFiles(tmp_s, tmp_d);
    updateBackupInfo(dest, src->d_name, &st_src);
  }
  
  return 0;
}


int updateBackupInfo(const char *dest, const char *name, const struct stat *st) {
  int info_file = 0;
  char path_info[PATH_MAX];
  strcpy(path_info, dest);
  strcat(path_info, "/");
  strcat(path_info, "__bckpinfo__");
  
  if ((info_file = open(path_info, O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IROTH)) == -1) {
    perror("open()");
    exit(-1);
  }
  
  // if it isn't in the begining of the file, add space after the last access
  if (lseek(info_file,0,SEEK_CUR) != 0) {
    write(info_file, "\n\n", 2);
  }
  
  // write basic information about the file:
  
  // name of the file
  write(info_file, ident_name, size_ident_name);
  
  // owner's name
  write(info_file, ident_owner, size_ident_owner);
  struct passwd *unm;
  if ((unm = getpwuid(st->st_uid)) == NULL) {
    perror("getpw()");
    exit(-1);
  }
  write(info_file, unm->pw_name, strlen(unm->pw_name));
  write(info_file, "\n", 1);
  
  // files's size
  write(info_file, ident_size, size_ident_size);
  char fs[TIME_LENGTH];
  int fs_size = sprintf(fs, "%d", (int)st->st_size);
  write(info_file, fs, fs_size);
  write(info_file, "\n", 1);
  
  
  write(info_file, ident_modified, size_ident_modified);
  
  struct tm *tm_file;
  if ((tm_file = localtime(&(st->st_mtime))) == NULL) {
    perror("localtime()");
    exit(-1);
  }
  
  char time[TIME_LENGTH];
  int time_size = sprintf(time, "%d_%d_%d_%d_%d_%d", tm_file->tm_year + 1900, tm_file->tm_mon + 1,
			  tm_file->tm_mday, tm_file->tm_hour, tm_file->tm_min, tm_file->tm_sec );
  write(info_file, time, time_size);
  
  return 0;
}

int incrementalBackup(char * dest) {
  
  return 0;
}

void exitHandler(void) {
  free(pathS);
  free(pathD);
  
  closedir(dirS);
  closedir(dirD);
}