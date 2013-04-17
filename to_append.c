

int sortDirectories() {
  int i = 0;
  int j = 0;      
  char *dirI = malloc(sizeof(char) * TIME_LENGTH);
  char *dirJ = malloc(sizeof(char) * TIME_LENGTH);
  char *dirTemp = malloc(sizeof(char) * TIME_LENGTH);
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
