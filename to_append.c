

int sortDirectories() {
  int i = 0;
  int j = 0;      
  char dirI[PATH_MAX];
  char dirJ[PATH_MAX];
  char dirTemp[PATH_MAX];
  if (bckp_directories != NULL) {
    for(i = 0;bckp_directories[i] != NULL;j++) {
      for(j = 0; bckp_directories[j] != NULL;j++) {
	
	if (bckp_directories[i] != bckp_directories[j]) {
	  
	  strcpy(dirI, basename(bckp_directories[i]));
	  strcpy(dirJ, basename(bckp_directories[j]));
	  
	  if (strcmp(dirI,dirJ) < 0) {
	    
	    strcpy(dirTemp,bckp_directories[i]);
	    strcpy(bckp_directories[i],bckp_directories[j]);
	    strcpy(bckp_directories[j],dirTemp);
	    
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

int readBckpInfo(char *filePath) {
  
  int i=0;
  char fileName[PATH_MAX];
  char fileNameS[PATH_MAX];
  strcpy(fileName, basename(filePath);
  struct dirent *dirBckp;
  DIR *dateFolder;
  
  for (i=0; bckp_directories[i] != NULL; i++) {
    
    dateFolder = opendir(bckp_directories[i]);
    while ((dirBckp = readdir(dateFolder)) != NULL) {
      
      if (strcmp(fileName,dirBckp->d_name) == 0) {
	strcpy(fileNameS,pathS);
	strcat(fileNameS,"/");
	strcat(fileNameS,fileName);
	
	return isFileModified(fileNameS,filePath);
	
      }
    }
  }
}








