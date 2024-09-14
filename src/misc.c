//
// Created by crystal on 14/09/24.
//

#include "misc.h"
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

bool pretend = 0;
const char *stratas[] = {"arch", "debian", "fedora", "ubuntu", "voidlinux"};
const char *filesystems[] = {"sys-fs/xfsprogs",   "sys-fs/e2fsprogs",
                             "sys-fs/dosfstools", "sys-fs/btrfs-progs",
                             "sys-fs/zfs",        "sys-fs/jfsutils"};
void exec_prog(char *command) {
  printf("Executing %s\n", command);
  if (pretend == 0) {
    if (system(command) != 0) {
      printf("Something went wrong\n");
      if (errno == EACCES)
        printf("Insufficient permissions, rerun with superuser perms\n");
      exit(EXIT_FAILURE);
    }
  }
}
void exec_prog_ignore_fail(char *command) {
  printf("Executing %s\n", command);
  if (pretend == 0) {
    if (system(command) != 0) {
      printf("Something went wrong\n");
      if (errno == EACCES) {
        printf("Insufficient permissions, rerun with superuser perms\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

FILE *openfile(const char *filename, const char *mode) {
  printf("\nOpening: %s\n", filename);
  if (pretend == 1)
    return stdout;
  FILE *openfile = fopen(filename, mode);
  if (openfile == NULL) {
    printf("\n Can't open %s\n", filename);
    exit(EXIT_FAILURE); // or abort()
  }
  return openfile;
}

void free_install(const install_type current) {
  free(current.filename);
  free(current.gpus);
  free(current.grub_disk);
  free(current.hostname);
  free(current.keyboard);
  free(current.locale);
  free(current.locales);
  free(current.rootpasswd);
  free(current.timezone);
  free(current.useflags);
  free(current.username);
  free(current.userpasswd);
  free(current.packages);
}
bool is_uefi() {
  DIR *dir = opendir("/sys/firmware/efi");
  if (dir) {
    closedir(dir);
    return true;
  }
  if (ENOENT == errno)
    return false;
  printf("Errno :%d, cant detect if system is UEFI or BIOS\n", errno);
  exit(EXIT_FAILURE);
}
