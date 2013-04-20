#include "common.h"
#include "headers.h"

// path of the dource 
char *pathS = NULL;
char *pathD = NULL;
DIR *dirS = NULL;
DIR *dirD = NULL;

const char CURR_DIR[] = ".";
const char FATHER_DIR[] = "..";

char **bckp_directories = NULL;

int copyFiles(const char *path_s, const char *path_d) {
  
  if (isFileTemp(path_s) == -1) {
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
    
    if (!verifyIfValidFolder(tmp)) {
      continue;
    }
    
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

int createProcess(const char *path_s, const char *path_d) {
  
  pid_t pid;
  
  pid = fork();
  
  if (pid == -1) {	//if fork returned an error
    perror("fork()");
    exit(-1);
  }
  else if (pid != 0) {		//if parent process wait for child to terminate
    return 0;
  }
  else {		//if child process executes the backup
    copyFiles(path_s, path_d);
    exit(0);
  }
  
  return 0;
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

int verifyIfValidFolder(char *pathname) {
  char tmp[PATH_MAX];
  strcpy(tmp, basename(pathname));
  if (pathname == NULL) {
    return 0;
  } else {
    int i = 0;
    int time[6];
    if ((i = sscanf(tmp, "%d_%d_%d_%d_%d_%d", &time[0], &time[1], &time[2], &time[3], &time[4], &time[5])) == 6) {
      return -1;
    } else {
      return 0;
    }
  }
}