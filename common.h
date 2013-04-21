#ifndef COMMON_H
#define COMMON_H

#include "headers.h"

/**
 * macros
 */
#define MAXBUFFER 1000
#define TIME_LENGTH 20
#define NR_TIME_ELEMS 20
#define _GNU_SOURCE
#define MAX_NR_FOLDERS 64000

//copies the file from path_s to path_d
int copyFiles(const char *path_s, const char *path_d);
//loads all directories in the backup folder to fill bckp_directories only if their name is valid
int loadDestDirectories();
//sorts the directories loaded to bckp_directories by descending order
int sortDirectories();
//loads a line from the given file to str
int loadLine(int file_desc, char *str);

char **loadPrevExistFiles(char *info_path);
//creates a new process to copy a file
int createProcess(const char *path_s, const char *path_d);
int isFileTemp(const char *pathname);
//verifies if a given folder is of the type yyyy_mm_dd_hh_mm_ss
int verifyIfValidFolder(char *pathname);

#endif