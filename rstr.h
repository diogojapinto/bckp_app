#ifndef RSTR_H
#define RSTR_H

//fills the four primary structures declared in rstr.c
int fillStructures();
//fills the existing_files structure and files_location
void fillExistingFiles(char *path);
//fills the files_on_folder structure
void fillFilesOnFolder(char *path, int i);
//exit handler to free the dinamically alocated memory
void exitHandler(void);
//displays the different backup points and and reads a choice
int askTimeFrame();
//depending on the return of askTimeFrame searches for files and restores them
void restoreBckpFiles(int index);
//to handle the different process created by restoreBckpFiles
void chldHandler(int signo);

#endif