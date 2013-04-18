char **loadPrevExistFiles(char *info_path) {
  char **old_filepaths = malloc(sizeof(char *) * MAX_NR_FOLDERS);
  char line[PATH_MAX];
  int info_desc;
  int i = 0;
  if ((info_desc = open(info_path)) == NULL) {
    perror("open()");
    exit(-1);
  }
  while(loadLine(info_desc, line) != -1) {
    if (strlen(line) != 1) {
      char tag[17];
      char file_name[PATH_MAX];
      sscanf("<%s> %s", tag, file_name);
      if (srtcmp(tag, "name") == 0) {
	old_filepaths[i] = malloc(sizeof(char) * PATH_MAX);
	strcpy(old_filepaths[i], file_name);
	i++;
      }
    }
  }
  return old_filepaths;
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
      return 0;
    } else {
      str[i] = c;
      i++;
    }
  }
}