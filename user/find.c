#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
/**
 * struct stat st;
 * int fd = open(path, 0)
 * fstat(fd, &st)
 *
 * st.type: T_DIR, T_FILE,...
 *
 * struct dirent entry;
 * read(fd, &entry, sizeof(entry) == sizeof(entry)) {
 *  memmove(p, de.name, DIRSIZ);
 * }
 */

int TRUE = 1;
int FALSE = 0;

int
has_prefix(char *prefix, char *name) {
  int len = strlen(prefix);

  if (strlen(name) < len) {
    return FALSE;
  }
  for (int i = 0; i < len; i++) {
    if (prefix[i] != name[i]) {
      return FALSE;
    }
  }
  return TRUE;
}

void
find(char *dir_path, char *target, char *prefix){

  struct stat st;
  struct dirent entry;

  int fd;
  char *p;
  char buffer[512];
  if ((fd = open(dir_path, 0)) < 0) {
    fprintf(2, "find: cannot open %s\n", dir_path);
    return;
  }


  while (read(fd, &entry, sizeof(entry)) == sizeof(entry)) {

    if (entry.inum == 0) {
      continue;
    }
    if (strcmp(entry.name, ".") == 0 || strcmp(entry.name, "..") == 0) {
      continue;
    }
    strcpy(buffer, dir_path);
    p = buffer + strlen(buffer);
    *p++ = '/';
    strcpy(p, entry.name);
    p[DIRSIZ] = 0;
    int fd_child = open(buffer, 0);
    fstat(fd_child, &st);
    close(fd_child);
    switch(st.type) {
      case T_DIR:
        find(buffer, target, prefix);
        break;
      case T_FILE:
        if (strcmp(entry.name, target) == 0 && has_prefix(prefix, buffer)){
          printf("%s\n", buffer);
        }
        break;
    }

  }
  close(fd);
}

int
main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Invald arguments");
    exit(1);
  }
  char *target = argv[1];
  char *dir = argv[2];
  find(".", dir, target);
  exit(0);
}

