
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

clock_t start, end;
struct tms t;

// array with the time (= basename(pathname)) of each backup
char **time_folders;

// array with the files discriminated in the __bckpinfo__ on each time of backup
char ***files_on_folder;

// array with the agregation of all the files saved through the backup
char **existing_files;

// array with the existing files' location (folder) on each frame of backup (it was modified or created at that time)
char *** files_location;

int main(int argc, char **argv) {
  //setbuf(stdout, NULL);
  
  // sets the umask, to make shure it is possible to create the files as the
  // current user
  umask(~(S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP |S_IROTH));
  
  // sets the exit handler that frees the dinamically allocated memory and closes open directories
  if (atexit(exitHandler)) {
    perror("exit()");
    exit(-1);
  }
  
  /**
   * arguments verification
   */
  
  // if the program was not used correctly:
  if (argc != 3) {
    printf("usage: %s <source-directory> <destination-directory> \n",argv[0]);
    exit(-1);
  }
  
  // cuts the paths untill the last directory on it, get the absolute paths, and 
  //verify them
  
  pathS = malloc(sizeof(char) * PATH_MAX);
  pathD = malloc(sizeof(char) * PATH_MAX);
  strcpy(pathS, argv[2]);
  strcpy(pathD, argv[1]);
  
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
  
  fillStructures();
  
  int index =  askTimeFrame();
  restoreBckpFiles(index);
  
  return 0;
}

void fillExistingFiles(char *path) {
  
  DIR *bckp_dir;
  struct dirent *bckp = NULL;
  existing_files = malloc(sizeof(char*) * MAX_NR_FOLDERS);
  
  if ((bckp_dir = opendir(path)) == NULL) {
    perror("opendir()");
    exit(-1);
  }
  
  // read the time frames backup directories
  while ((bckp = readdir(bckp_dir)) != NULL) {
    
    struct stat st_bckp;
    
    // prepares the pathnames
    char tmp_b[PATH_MAX];
    strcpy(tmp_b, path);
    strcat(tmp_b,"/");
    strcat(tmp_b,bckp->d_name);
    
    // verifies if the  file is regular
    if (lstat(tmp_b, &st_bckp) == -1) {
      perror("lstat()");
      exit(-1);
    }
    if (!S_ISREG(st_bckp.st_mode)) {
      continue;
    }
    
    int j = 0;
    int found = 0;
    char base[PATH_MAX];
    char base1[PATH_MAX];
    for (j=0; existing_files[j] != NULL; j++) {
      strcpy(base1, basename(tmp_b));		//procura o nome no existing_files, se nao encontrar adiciona ao existing_files
      strcpy(base, basename(existing_files[j]));
      if ((strcmp(base,base1) == 0) || (strcmp(base1,"__bckpinfo__") == 0)){
	found = -1;
	int i = 0;
	while (files_location[j][i] != NULL) {
	  i++;
	}
	files_location[j][i] = malloc(sizeof(char) * PATH_MAX);
	strcpy(files_location[j][i], basename(path));
	break;
      }
    }
    if (found == 0) {
      existing_files[j] = malloc(sizeof(char) * PATH_MAX);
      strcpy(existing_files[j], basename(tmp_b));
      files_location[j] = malloc(sizeof(char*) * MAX_NR_FOLDERS);
      files_location[j][0] = malloc(sizeof(char) * PATH_MAX);
      strcpy(files_location[j][0], basename(path));
    }
  }
}

void fillFilesOnFolder(char *path, int i) {
  
  char line[PATH_MAX];
  int fd=0;
  int j=0;
  if ((fd = open(path, O_RDONLY)) == -1) {
    perror("open()");
    exit(-1);
  }
  
  files_on_folder[i] = malloc(sizeof(char*) * MAX_NR_FOLDERS);
  
  while (loadLine(fd, line) != -1) {
    if (strlen(line) != 1) {
      char tag[17];
      char file_name[PATH_MAX];
      sscanf(line, "%s %[^\n]s", tag, file_name);
      if (strcmp(tag, "<name>") == 0) {
	files_on_folder[i][j] = malloc(sizeof(char) * PATH_MAX);	//le a linha se for nome adiciona a ij
	strcpy(files_on_folder[i][j], file_name);
	j++;
      }
    }
  }
}

int fillStructures() {
  
  // initializes the base pointers of the structures
  loadDestDirectories();
  
  time_folders = malloc(sizeof(char*) * MAX_NR_FOLDERS);
  files_on_folder = malloc(sizeof(char**) * MAX_NR_FOLDERS);
  existing_files = malloc(sizeof(char*) * MAX_NR_FOLDERS);
  files_location = malloc(sizeof(char**) * MAX_NR_FOLDERS);
  
  int i = 0;
  for(i=0; bckp_directories[i] != NULL; i++) {
    
    //fill time_folders
    time_folders[i] = malloc(sizeof(char) * PATH_MAX);
    strcpy(time_folders[i], basename(bckp_directories[i]));
    
    // saves the pathname of the bckpinfo file for use
    char tmp[PATH_MAX];
    sprintf(tmp, "%s\%s", bckp_directories[i], "/__bckpinfo__");
    
    //fill files_on_folder
    fillFilesOnFolder(tmp,i);
    
    //fill existing_files
    fillExistingFiles(bckp_directories[i]);
  }
  
  return 0;
}

int askTimeFrame() {
  long choice;
  write(STDOUT_FILENO, "List of available restore points (yyyy-mm-dd-hh-mm-ss)\n", 56);
  int i = 0;
  for (i = 0; time_folders[i] != NULL; i++) {
    char tmp[5];
    sprintf(tmp, "(%d) ", i + 1);
    write(STDOUT_FILENO, tmp, 5);
    write(STDOUT_FILENO, time_folders[i], strlen(time_folders[i]) + 1);
    write(STDOUT_FILENO, "\n", 2);
  }
  
  while(1) {
    char s[5];
    choice = 0;
    read(STDIN_FILENO, s, 5);
    choice = strtol(s, NULL, 10);
    if (choice == 0 || choice > i) {
      write(STDOUT_FILENO, "Invalid input\n", 15);
    } else {
      break;
    }
  }
  return choice - 1;
}

void restoreBckpFiles(int index) {
  // isntalls the SIGCHLD signal handler
  struct sigaction sigchld_handler;
  sigchld_handler.sa_handler = chldHandler;
  
  sigaction(SIGCHLD, &sigchld_handler, NULL);
  
  // records the time taken by the task
  start = times(&t);
  long ticks = sysconf(_SC_CLK_TCK);
  
  // saves locally the folder path
  char time_frame[PATH_MAX];
  strcpy(time_frame, time_folders[index]);
  
  // for each file saved at that moment (on __bckpinfo__)
  int i = 0;
  for(; files_on_folder[index][i] != NULL; i++) {
    // find the locations of that file
    int files_index = 0;
    while(existing_files[files_index] != NULL) {
      if (strcmp(existing_files[files_index], files_on_folder[index][files_index]) == 0) {
	break;
      } else {
	files_index++;
      }
    }
    // and verify which is the location equal or immediately before that time frame
    // on which the file was saved
    int k = 0;
    while (files_location[files_index][k] != NULL) {
      // as the folders are ordered from newest to oldest, the first folder to 
      // hold the file and have the time equal or before the time of desired backup
      // is the file to be copied
      int is_data_valid = strcmp(files_location[files_index][k], time_frame);
      if (is_data_valid <= 0) {
	break;
      } else {
	k++;
      }
    }
    
    // now copy the files
    char src_file[PATH_MAX];
    char dest_file[PATH_MAX];
    sprintf(src_file, "%s/%s/%s", pathD, files_location[files_index][k], files_on_folder[index][i]);
    sprintf(dest_file, "%s/%s", pathS,  files_on_folder[index][i]);
    
    printf("%s\n%s\n\n", src_file, dest_file);
    
    createProcess(src_file, dest_file);
  }
  
  // print information about the time it took to the process
  end = times(&t);
  printf("Restore done in %4.2f seconds.\n", (double)(end - start) / ticks);
  printf("Main process took %4.2f seconds (%4.2f user, %4.2f system).\n", (double)(t.tms_utime +  t.tms_stime) / ticks, (double)t.tms_utime/ ticks, (double)t.tms_stime / ticks);
  printf("Children took %4.2f seconds globaly (%4.2f user, %4.2f system).\n", (double)(t.tms_cutime +  t.tms_cstime) / ticks, (double)t.tms_cutime/ ticks, (double)t.tms_cstime / ticks);
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
  
  int i = 0, j = 0;
  
  for (i = 0; time_folders[i] != NULL; i++) {
    free(time_folders[i]);
  }
  free(time_folders);
  
  for (i = 0; existing_files[i] != NULL; i++) {
    free(existing_files[i]);
  }
  free(existing_files);
  
  for (i = 0; files_on_folder[i] != NULL; i++) {
    for (j = 0; files_on_folder[i][j] != NULL; j++) {
      free(files_on_folder[i][j]);
    }
    free(files_on_folder[i]);
  }
  free(files_on_folder);
  
  for (i = 0; files_location[i] != NULL; i++) {
    for (j = 0; files_location[i][j] != NULL; j++) {
      free(files_location[i][j]);
    }
    free(files_location[i]);
  }
  free(files_location);  
}