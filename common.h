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

int copyFiles(const char *path_s, const char *path_d);
int loadDestDirectories();
int sortDirectories();
int loadLine(int file_desc, char *str);
char **loadPrevExistFiles(char *info_path);
int createProcess(const char *path_s, const char *path_d);
int isFileTemp(const char *pathname);
int verifyIfValidFolder(char *pathname);

#endif