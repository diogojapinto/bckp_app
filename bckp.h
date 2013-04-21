#ifndef BCKP_H
#define BCKP_H

#include "headers.h"

#define FILES_EQUAL 0 
#define FILES_DIFFERENT 1
#define FILES_DELETED 1
#define NO_FILES_DELETED 0

/*
 * generates the backup destination folder with the format yyyy_mm_dd_hh_mm_ss
 * 
 * returns a pointer to an allocated char array
 */
char* createDestFolderName();

/*
 * verifies if there is any difference between the file with the path 'path_s'
 * and the file with the path 'path_d'
 * 
 * return FILES_DIFFERENT if there is differences, 0 FILES_EQUAL
 */
int isFileModified(const char *path_s, const char *path_d);

/*
 * performs the initial full backup, to the folder 'dest' (already with the time format)
 * uses pathS as the source folder from where to fetch for the files to save
 * 
 * calls createProcess
 * 
 * return -1 if error, 0 if successfull
 */
int fullBackup(char *dest);

/*
 * performs the incremental backup, to the folder 'dest' (already with the time format)
 * uses pathS as the source folder from where to fetch for the files to save
 * verifies if there is any file created, deleted or modified
 * 
 * calls createProcess()
 * 
 * return -1 if error, 0 if successfull
 */
int incrementalBackup(char * dest);

/*
 * adds to the file __bckpinfo__ information of the file 'name' with the state 'st'
 * if __bckpinfo__ doesn't exist, it is created
 * 
 * return -1 if error, 0 if successfull
 */
int updateBackupInfo(const char *dest, const char *name, const struct stat *st);

/*
 * tries to find the latest occurrence of a file with the name 'filePath'
 * if there is one, calls isFileModified() and returns it value
 * 
 */
int findPrevFile(char *filePath);

/*
 * creates the file __bckpinfo__ and fills it with the currently existing files, placing it on
 * the folder 'pathname'
 */
void createBckpInfo(const char *pathname);

/*
 * finds if there is any file that was deleted (discriminated on last __bckpinfo__ file but 
 * doesn't exist at the moment of the function call)
 * 
 * return FILES_DELETED if true, NO_FILES_DELETED otherwise
 */
int existsDeletedFiles();

/*
 * assings the handlers for the signals SIGALRM and SIGUSR1
 */
void installHandlers();

/*
 * updates a global variable when an alarm occurs
 */
void alarmHandler(int signo);

/*
 * updates a global variable when the process receives a SIGUSR1 signal
 */
void sigusr1Handler(int signo);

/*
 * calls wait() for a child process
 */
void chldHandler(int signo);

/*
 * defines a mask with all the signals (minus the unblockable ones) blocked, except SIGALRM and SIGUSR1
 */
int generateSignalMask(sigset_t *empty_mask);

/*
 * frees the dinamically allocated space and closes the directories
 */
void exitHandler(void);

#endif