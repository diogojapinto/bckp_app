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

char* createDestFolder();
int isFileModified(const char *path_s, const char *path_d);
int copyFiles(const char *path_s, const char *path_d);
int fullBackup(char *dest);
int incrementalBackup(char * dest);
int updateBackupInfo(const char *dest, const char *name, const struct stat *st);
int loadDestDirectories();
int sortDirectories();

// array of pathnames of the destination folders created
char **bckp_directories;

// path of the dource 
char *pathS;
char *pathD;
DIR *dirS;
DIR *dirD;

char ident_name[];
int size_ident_name;
char ident_owner[];
int size_ident_owner;
char ident_size[];
int size_ident_size;
char ident_modified[];
int size_ident_modified;