#include "bckp.h"
#include "common.h"
#include "headers.h"

extern const char CURR_DIR[];
extern const char FATHER_DIR[];
// array of pathnames of the destination folders created
extern char **bckp_directories;

// path of the dource 
extern char *pathS;
extern char *pathD;
extern DIR *dirS;
extern DIR *dirD;

int alarm_occurred = 0;
int exit_on_finish = 0;

const char ident_name[] = "<name> ";
const int size_ident_name = 7;
const char ident_owner[] = "<owner> ";
const int size_ident_owner = 8;
const char ident_size[] = "<size(bytes)> ";
const int size_ident_size = 14;
const char ident_modified[] = "<last_modified> ";
const int size_ident_modified = 16;


int main(int argc, char *argv[]) {
  //setbuf(stdout, NULL);
  
  // sets the umask, to make shure it is possible to create the files as the
  // current user
  //setbuf(stdout, NULL);
  umask(~(S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP |S_IROTH));
  
  // sets the exit handler that frees the dinamically allocated memory and closes open directories
  if (atexit(exitHandler)) {
    perror("exit()");
    exit(-1);
  }
  
  // call the function to initialize the signal handlers
  installHandlers();
  
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
  char* bckp_dest;
  bckp_dest = createDestFolderName();
  fullBackup(bckp_dest);
  
  sigset_t sigalarm;
  sigemptyset(&sigalarm);
  sigaddset(&sigalarm, SIGALRM);
  
  while(!exit_on_finish) {
    sigset_t sigset;
    generateSignalMask(&sigset);
    alarm(time_frame);
    pid_t pid;
    if ((pid = fork()) == -1) {
      perror("fork()");
      exit(-1);
    }
    
    sigprocmask(SIG_BLOCK, &sigalarm, NULL);
    if (pid > 0) {
      int ret;
      if (waitpid(pid, &ret, 0) == -1) {
	perror("waitpid()");
      }
      else {
	if (!WIFEXITED(ret)) {
	  write(STDOUT_FILENO,"Incremental backup failed\n", 27);
	}
      }
      sigprocmask(SIG_UNBLOCK, &sigalarm, NULL);
      if (alarm_occurred == 0) {
	sigsuspend(&sigset);
      }
      else if (alarm_occurred == -1) {
	alarm_occurred = 0;
      }
    } else {
      free(bckp_dest);
      bckp_dest = createDestFolderName();
      incrementalBackup(bckp_dest);
      exit(0);
    }
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
  
  if (sprintf(dir_name, "%.4d_%.2d_%.2d_%.2d_%.2d_%.2d", local_time->tm_year + 1900, 
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
 * but there is the possibility of a file being modified in a time 
 * interval 
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

int fullBackup(char * dest) {
  sigset_t a_sigset, o_sigset;
  sigemptyset(&a_sigset);
  sigaddset(&a_sigset,SIGCHLD);
  
  sigprocmask(SIG_UNBLOCK, &a_sigset, &o_sigset);
  
  struct sigaction sigchld_handler;
  sigchld_handler.sa_handler = chldHandler;
  
  sigaction(SIGCHLD, &sigchld_handler, NULL);
  
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
    createProcess(tmp_s, tmp_d);
    updateBackupInfo(dest, src->d_name, &st_src);
  }
  
  sigprocmask(SIG_UNBLOCK, &o_sigset, NULL);
  
  struct sigaction def_sigchld_handler;
  def_sigchld_handler.sa_handler = SIG_DFL;
  
  sigaction(SIGCHLD, &def_sigchld_handler, NULL);
  return 0;
}

int updateBackupInfo(const char *dest, const char *name, const struct stat *st) 
{
  char filePath[PATH_MAX];
  strcpy(filePath,dest);
  strcat(filePath, "/");
  strcat(filePath, name);
  if (isFileTemp(filePath) != -1) {
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
    int time_size = sprintf(time, "%.4d_%.2d_%.2d_%.2d_%.2d_%.2d", tm_file->tm_year + 1900, 
			    tm_file->tm_mon + 1,
			    tm_file->tm_mday, tm_file->tm_hour, 
			    tm_file->tm_min, 
			    tm_file->tm_sec );
    write(info_file, time, time_size);
    
    close(info_file);
    
    return 0;
  }
  return 0;
}

int incrementalBackup(char * dest) {
  // if 0, bckpDirectory not created
  // if -1, already exists
  int state_dest = 0;
  
  sigset_t sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGCHLD);
  
  sigprocmask(SIG_UNBLOCK, &sigset, NULL);
  
  struct sigaction sigchld_handler;
  sigchld_handler.sa_handler = chldHandler;
  
  sigaction(SIGCHLD, &sigchld_handler, NULL);
  
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
      createProcess(tmp_s, tmp_d);
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
  
  exit(0);
}

int findPrevFile(char *filePath) {
  
  int i=0;
  char fileName[PATH_MAX];
  char fileNameS[PATH_MAX];
  strcpy(fileName, basename(filePath));
  struct dirent *dirBckp;
  DIR *dateFolder;
  
  
  if (isFileTemp(filePath) == -1) {
    return FILES_EQUAL;
  }
  
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
      sscanf(line, "%s %[^n]s", tag, file_name);
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

void installHandlers() {
  
  
  sigset_t sigset;
  generateSignalMask(&sigset);
  
  struct sigaction new_alarm_handler;
  struct sigaction new_sigusr1_handler;
  
  new_alarm_handler.sa_mask = sigset;
  new_sigusr1_handler.sa_mask = sigset;
  
  new_alarm_handler.sa_handler = alarmHandler;
  if(sigaction(SIGALRM, &new_alarm_handler, NULL)) {
    perror("sigaction()");
    exit(-1);
  }
  
  new_sigusr1_handler.sa_handler = sigusr1Handler;
  if(sigaction(SIGUSR1, &new_sigusr1_handler, NULL)) {
    perror("sigaction()");
    exit(-1);
  }
}

void alarmHandler(int signo) {
  alarm_occurred = -1;
}

void sigusr1Handler(int signo) {
  exit_on_finish = -1;
}

int generateSignalMask(sigset_t *empty_mask) {
  sigfillset(empty_mask);
  sigdelset(empty_mask, SIGALRM);
  sigdelset(empty_mask, SIGUSR1);
  sigdelset(empty_mask, SIGKILL);
  sigdelset(empty_mask, SIGSTOP);
  
  return 0;
}

void chldHandler(int signo) {
  int ret = 0;
  wait(&ret);
  if (ret == -1) {
    write(STDOUT_FILENO, "Copy of files failed without possible recovery.\n", 49);
    exit(-1);
  }
}

void exitHandler(void) {
  free(pathS);
  free(pathD);
  free(bckp_directories);
  closedir(dirS);
  closedir(dirD);
  
  struct sigaction new_handler;
  new_handler.sa_handler = SIG_DFL;
  sigaction(SIGUSR1, &new_handler, NULL);
}