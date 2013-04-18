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
#define TIME_LENGTH 20
#define NR_TIME_ELEMS 20
#define _GNU_SOURCE
#define MAX_NR_FOLDERS 64000
#define FILES_EQUAL 0 
#define FILES_DIFFERENT 1
#define FILES_DELETED 1
#define NO_FILES_DELETED 0

const char CURR_DIR[] = ".";
const char FATHER_DIR[] = "..";


char* createDestFolderName();
int isFileModified(const char *path_s, const char *path_d);
int copyFiles(const char *path_s, const char *path_d);
int fullBackup(char *dest);
int incrementalBackup(char * dest);
int updateBackupInfo(const char *dest, const char *name, const struct stat *st);
int loadDestDirectories();
int sortDirectories();
int findPrevFile(char *filePath);
int isFileTemp(const char *pathname);
void createBckpInfo(const char *pathname);
int loadLine(int file_desc, char *str);
char **loadPrevExistFiles(char *info_path);
int createBckpInfoDel();

// array of pathnames of the destination folders created
char **bckp_directories = NULL;

// path of the dource 
char *pathS = NULL;
char *pathD = NULL;
DIR *dirS = NULL;
DIR *dirD = NULL;
/**
 * TO ADD:
 * -> the sleep is done in a process, and copy by other... verify if copy is done (handler for sigchld)
 * don't forget the return value of sleep (if != 0, do sleep/ALARM of remaining time)
 */


char ident_name[] = "<name> ";
int size_ident_name = 7;
char ident_owner[] = "<owner> ";
int size_ident_owner = 8;
char ident_size[] = "<size(bytes)> ";
int size_ident_size = 14;
char ident_modified[] = "<last_modified> ";
int size_ident_modified = 16;


int main(int argc, char *argv[]) {
  
  // sets the umask, to make shure it is possible to create the files as the
  // current user
  //setbuf(stdout, NULL);
  umask(~(S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP |S_IROTH));
  
  /**
   * arguments verification
   */
  
  // if the program was not used correctly:
  if (argc != 4) {
    printf("usage: %s <source-directory> <destination-directory> ",argv[0]); 
    printf("<time-interval-between-backups>\n");
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
  
  int time_frame = atoi(argv[3]);
  
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
  
  /*if (dirD_exists == 0) {
   *   printf("created folder %s\n", pathD);
}*/
  
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
  
  // performs a full backup
  char* bckp_dest = createDestFolderName();
  fullBackup(bckp_dest);
  
  while(1) {
    sleep(time_frame);
    free(bckp_dest);
    bckp_dest = createDestFolderName();
    incrementalBackup(bckp_dest);
  }
  
  return 0;
}

/**
 * creates the destination folder for the backup, with the the name 
 * corresponding to the local time
 */
char* createDestFolderName() {
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
  
  if (sprintf(dir_name, "%d_%d_%d_%d_%d_%d", local_time->tm_year + 1900, 
    local_time->tm_mon + 1,
    local_time->tm_mday, local_time->tm_hour, local_time->tm_min, 
    local_time->tm_sec ) >= NR_TIME_ELEMS) {
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
    
    return tmp_dir;
}

/**
 * return 1 if files are different, 0 otherwise
 * 
 * (could have used the field st_mtime from the stat structure correspondent to 
 * t *h*e source files
 * but there is the possibility of a file being modified in a time interval 
 * lower than one second)
 */
int isFileModified(const char *path_s, const char *path_d) {
  int source, destination;
  char cs, cd;
  
  // tries to open the files
  if ((source = open(path_s, O_RDONLY) ) == -1) {
    perror("open()");
    exit(-1);
  }
  
  if ((destination = open(path_d, O_RDONLY)) == -1) {
    perror("open()");
    exit(-1);
  }
  
  do {
    // reads a char from the files
    int i = read(source, &cs, 1);
    int j = read(destination, &cd, 1);
    if (i == 0 && j == 0) {
      break;
    } else if (i != j) {
      close(source);
      close(destination);
      return FILES_DIFFERENT;
    } else if(cs != cd) {
      close(source);
      close(destination);
      return FILES_DIFFERENT;
    }
  } while ( cs != '\0' && cs != '\0');
  
  // closes the files compared
  close(source);
  close(destination);
  return FILES_EQUAL;
}

int copyFiles(const char *path_s, const char *path_d) {
  if (isFileTemp(path_s)) {
    return 0;
  }
  int source, destination;
  char c;
  
  // tries to open the files
  if ((source = open(path_s, O_RDONLY) ) == -1) {
    perror("open()");
    exit(-1);
  }
  
  if ((destination = open(path_d, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | 
    S_IXUSR | S_IRGRP | S_IROTH)) == -1) {
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
  rewinddir(dirD);
  
  if (mkdir(dest, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IROTH)) {
    perror("mkdir()");
    exit(-1);
  }
  
  while((src = readdir(dirS)) != NULL) {
    struct stat st_src;
    
    // prepares the pathnames for source file
    char tmp_s[PATH_MAX];
    strcpy(tmp_s, pathS);
    strcat(tmp_s,"/");
    strcat(tmp_s,src->d_name);
    
    // verifies if the source file is regular
    if (lstat(tmp_s, &st_src) == -1) {
      perror("lstat()");
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

int updateBackupInfo(const char *dest, const char *name, const struct stat *st) 
{
  int info_file = 0;
  char path_info[PATH_MAX];
  strcpy(path_info, dest);
  strcat(path_info, "/");
  strcat(path_info, "__bckpinfo__");
  
  int file_existed = 0;
  if ((info_file = open(path_info, O_RDONLY)) != -1) {
    file_existed = 1;
  }
  
  close(info_file);
  
  if ((info_file = open(path_info, O_RDWR | O_CREAT | O_APPEND, S_IRUSR | 
    S_IWUSR | S_IXUSR | S_IRGRP | S_IROTH)) == -1) {
    perror("open()");
  exit(-1);
    }
    
    // if it isn't in the begining of the file, add space after the last access
    //if (lseek(info_file,0,SEEK_CUR) == 0) {
    if (file_existed) {
      write(info_file, "\n\n", 2);
    }
    
    // write basic information about the file:
    
    // name of the file
    write(info_file, ident_name, size_ident_name);
    write(info_file, name, strlen(name));
    write(info_file, "\n", 1);
    
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
    
    // time of last modification
    write(info_file, ident_modified, size_ident_modified);
    
    struct tm *tm_file;
    if ((tm_file = localtime(&(st->st_mtime))) == NULL) {
      perror("localtime()");
      exit(-1);
    }
    
    char time[TIME_LENGTH];
    int time_size = sprintf(time, "%d_%d_%d_%d_%d_%d", tm_file->tm_year + 1900, 
			    tm_file->tm_mon + 1,
			    tm_file->tm_mday, tm_file->tm_hour, 
			    tm_file->tm_min, 
			    tm_file->tm_sec );
    write(info_file, time, time_size);
    
    close(info_file);
    
    return 0;
}

int incrementalBackup(char * dest) {
  // if 0, bckpDirectory not created
  // if -1, already exists
  int state_dest = 0;
  
  if (bckp_directories != NULL) {
    int i = 0;
    while(bckp_directories[i] != NULL) {
      free(bckp_directories[i]);
      i++;
    }
    free(bckp_directories);
  }
  loadDestDirectories();
  
  struct dirent *src = NULL;
  rewinddir(dirS);
  while((src = readdir(dirS)) != NULL) {
    struct stat st_src;
    
    // prepares the pathnames for source file
    char tmp_s[PATH_MAX];
    strcpy(tmp_s, pathS);
    strcat(tmp_s,"/");
    strcat(tmp_s,src->d_name);
    
    // verifies if the source file is regular
    if (lstat(tmp_s, &st_src) == -1) {
      perror("lstat()");
      exit(-1);
    }
    
    if (S_ISREG(st_src.st_mode)) {
    } else {
      continue;
    }
    
    // prepares the pathnames for dest file
    char tmp_d[PATH_MAX];
    strcpy(tmp_d, dest);
    strcat(tmp_d,"/");
    strcat(tmp_d,src->d_name);
    
    if (findPrevFile(tmp_s) == FILES_DIFFERENT) {
      if (state_dest == 0) {
	if (mkdir(dest, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IROTH)) {
	  perror("mkdir()");
	  exit(-1);
	}
	state_dest = -1;
      }
      copyFiles(tmp_s, tmp_d);
    }
  }
  
  if (state_dest == -1) {
    createBckpInfo(dest);
  }
  else if (state_dest == 0) {
    if (createBckpInfoDel() == FILES_DELETED) {
      if (mkdir(dest, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IROTH)) {
	perror("mkdir()");
	exit(-1);
      }
      createBckpInfo(dest);
    }
  }
  
  return 0;
}



int loadDestDirectories() {
  bckp_directories = malloc(sizeof(char*) * MAX_NR_FOLDERS);
  int i = 0;
  rewinddir(dirD);
  struct dirent *dest_f;
  while((dest_f = readdir(dirD)) != NULL) {
    if (strcmp(dest_f->d_name, CURR_DIR) == 0 || strcmp(dest_f->d_name, 
      FATHER_DIR) == 0) {
      continue;
      }
      char tmp[PATH_MAX];
    strcpy(tmp, pathD);
    strcat(tmp, "/");
    strcat(tmp, dest_f->d_name);
    
    struct stat st;
    if (lstat(tmp, &st)) {
      perror("lstat()");
      exit(-1);
    }
    
    if (S_ISDIR(st.st_mode)) {
      bckp_directories[i] = malloc(sizeof(char) * PATH_MAX);
      strcpy(bckp_directories[i], tmp);      
    } else {
      continue;
    }
    
    ++i;
  }
  
  bckp_directories[i] = NULL;
  
  return sortDirectories();
}

int sortDirectories() {
  int i = 0;
  int j = 0;      
  char dirTemp[PATH_MAX];
  if (bckp_directories != NULL) {
    for(i = 0;bckp_directories[i + 1] != NULL; i++) {
      for(j = i + 1; bckp_directories[j] != NULL; j++) {
	int time_i[6];
	int time_j[6];
	char dirI[PATH_MAX];
	strcpy(dirI, basename(bckp_directories[i]));
	char dirJ[PATH_MAX];
	strcpy(dirJ, basename(bckp_directories[j]));
	sscanf(dirI, "%d_%d_%d_%d_%d_%d", &time_i[0], &time_i[1], &time_i[2], 
	       &time_i[3], &time_i[4], &time_i[5]);
	sscanf(dirJ, "%d_%d_%d_%d_%d_%d", &time_j[0], &time_j[1], &time_j[2], 
	       &time_j[3], &time_j[4], &time_j[5]);
	int a;
	for (a = 0; a < 6; a++) {
	  if (time_i[a] < time_j[a]) {
	    strcpy(dirTemp, bckp_directories[i]);
	    strcpy(bckp_directories[i], bckp_directories[j]);
	    strcpy(bckp_directories[j], dirTemp);
	    break;
	  } else if (time_i[a] > time_j[a]) {
	    break;
	  }
	}
      }
    }
    
    return 0;
  }
  else {
    return -1;
  }
  
}

int findPrevFile(char *filePath) {
  
  int i=0;
  char fileName[PATH_MAX];
  char fileNameS[PATH_MAX];
  strcpy(fileName, basename(filePath));
  struct dirent *dirBckp;
  DIR *dateFolder;
  
  for (i=0; bckp_directories[i] != NULL; i++) {
    
    dateFolder = opendir(bckp_directories[i]);
    while ((dirBckp = readdir(dateFolder)) != NULL) {
      
      if (strcmp(fileName,dirBckp->d_name) == 0) {
	strcpy(fileNameS,bckp_directories[i]);
	strcat(fileNameS,"/");
	strcat(fileNameS,fileName);
	return isFileModified(fileNameS,filePath);
      }
    }
    closedir(dateFolder);
  }
  // return the code used to tell that the file has to be copied
  return FILES_DIFFERENT;
}

/*
 * return 0 if false, -1 if true
 */
int isFileTemp(const char *pathname) {
  if (pathname == NULL) {
    return 0;
  } else {
    if (pathname[strlen(pathname) - 1] == '~') {
      return -1;
    } else {
      return 0;
    }
  }
}

int createBckpInfoDel() {
  char path_info[PATH_MAX];
  strcpy(path_info, bckp_directories[0]);
  strcat(path_info, "/");
  strcat(path_info, "__bckpinfo__");
  char** bckpInfoFiles = loadPrevExistFiles(path_info);
  
  int i = 0;
  for (i=0; bckpInfoFiles[i] != NULL;i++) {
    int found = 0;
    struct dirent *src = NULL;
    rewinddir(dirS);
    while((src = readdir(dirS)) != NULL) {
      struct stat st_src;
      
      // prepares the pathnames for source file
      char tmp_s[PATH_MAX];
      strcpy(tmp_s, pathS);
      strcat(tmp_s,"/");
      strcat(tmp_s,src->d_name);
      
      // verifies if the source file is regular
      if (lstat(tmp_s, &st_src) == -1) {
	perror("lstat()");
	exit(-1);
      }
      
      if (S_ISREG(st_src.st_mode)) {
      } else {
	continue;
      }
      
      if (strcmp(bckpInfoFiles[i],src->d_name) == 0) {
	found = -1; 
	break;
      }
    }
    if (!found) {
      int a = 0;
      while(bckpInfoFiles[a] != NULL) {
	free(bckpInfoFiles[a]);
	a++;
      }
      free(bckpInfoFiles);
      return FILES_DELETED;
    }
  }
  
  int a = 0;
  while(bckpInfoFiles[a] != NULL) {
    free(bckpInfoFiles[a]);
    a++;
  }
  free(bckpInfoFiles);
  return NO_FILES_DELETED;
}

char **loadPrevExistFiles(char *info_path) {
  char **old_filepaths = malloc(sizeof(char *) * MAX_NR_FOLDERS);
  char line[PATH_MAX];
  int info_desc;
  int i = 0;
  if ((info_desc = open(info_path, O_RDONLY)) == -1) {
    perror("open()");
    exit(-1);
  }
  while(loadLine(info_desc, line) != -1) {
    if (strlen(line) != 1) {
      char tag[17];
      char file_name[PATH_MAX];
      sscanf(line, "%s %s", tag, file_name);
      if (strcmp(tag, "<name>") == 0) {
	old_filepaths[i] = malloc(sizeof(char) * PATH_MAX);
	strcpy(old_filepaths[i], file_name);
	i++;
      }
    }
  }
  old_filepaths[i]=NULL;
  return old_filepaths;
}

/*
 * return 0 if end of line
 * return -1 if end of file
 */
int loadLine(int file_desc, char *str) {
  char c;
  int i = 0;
  int size = 0;
  while(1) {
    size = read(file_desc, &c, 1);
    if (size == 0) {
      return -1;
    }
    if (c == '\n') {
      str[i] = '\0';
      return 0;
    } else {
      str[i] = c;
      i++;
    }
  }
}

void createBckpInfo(const char *pathname) {
  struct dirent *src = NULL;
  rewinddir(dirS);
  while((src = readdir(dirS)) != NULL) {
    struct stat st_src;
    
    // prepares the pathnames for source file
    char tmp_s[PATH_MAX];
    strcpy(tmp_s, pathS);
    strcat(tmp_s,"/");
    strcat(tmp_s,src->d_name);
    
    // verifies if the source file is regular
    if (lstat(tmp_s, &st_src) == -1) {
      perror("lstat()");
      exit(-1);
    }
    
    if (S_ISREG(st_src.st_mode)) {
    } else {
      continue;
    }
    
    // prepares the pathnames for dest file
    char tmp_d[PATH_MAX];
    strcpy(tmp_d, pathname);
    strcat(tmp_d,"/");
    strcat(tmp_d,src->d_name);
    updateBackupInfo(pathname, src->d_name, &st_src);
  }
}

void exitHandler(void) {
  free(pathS);
  free(pathD);
  free(bckp_directories);
  closedir(dirS);
  closedir(dirD);
}