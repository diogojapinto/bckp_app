#ifndef BCKP_H
#define BCKP_H

#include "headers.h"

#define FILES_EQUAL 0 
#define FILES_DIFFERENT 1
#define FILES_DELETED 1
#define NO_FILES_DELETED 0

char* createDestFolderName();
int isFileModified(const char *path_s, const char *path_d);
int fullBackup(char *dest);
int incrementalBackup(char * dest);
int updateBackupInfo(const char *dest, const char *name, const struct stat *st);
int findPrevFile(char *filePath);
void createBckpInfo(const char *pathname);
int createBckpInfoDel();
void installHandlers();
void alarmHandler(int signo);
void sigusr1Handler(int signo);
void chldHandler(int signo);
int generateSignalMask(sigset_t *empty_mask);
void exitHandler(void);

#endif