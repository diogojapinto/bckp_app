#ifndef RSTR_H
#define RSTR_H

int fillStructures();
void fillExistingFiles(char *path);
void fillFilesOnFolder(char *path, int i);
void exitHandler(void);
int askTimeFrame();
void restoreBckpFiles(int index);
void chldHandler(int signo);

#endif