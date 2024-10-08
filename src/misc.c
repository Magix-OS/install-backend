

#include "misc.h"
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>
/*========================Global Variables========================*/
// Set to true to just output the steps that would be made, and false to make
// changes to the disk
bool pretend = false;
// Which Stratas to install (Bedrock Linux related) TODO actually use it
const char *stratas[STRATAS_NUMBER] = {"arch", "debian", "fedora", "ubuntu",
                                       "voidlinux"};
// Which filesystem packages to install
const char *filesystems[FS_NUMBER] = {"sys-fs/xfsprogs",   "sys-fs/e2fsprogs",
                                      "sys-fs/dosfstools", "sys-fs/btrfs-progs",
                                      "sys-fs/zfs",        "sys-fs/jfsutils"};
/*========================Global Variables========================*/

// Wrapper around the system() function. Takes into consideration
// the pretend global variable
void exec_prog(char *command) {
  printf("Executing %s\n", command);
  if (pretend == false) {
    if (system(command) != 0) {
      printf("Something went wrong\n");
      if (errno == EACCES)
        printf("Insufficient permissions, rerun with superuser perms\n");
      exit(EXIT_FAILURE);
    }
  }
}
// Same as before but ignore the case where the command exists with a non 0
// status
void exec_prog_ignore_fail(char *command) {
  printf("Executing %s\n", command);
  if (pretend == false) {
    if (system(command) != 0) {
      printf("Something went wrong\n");
      if (errno == EACCES) {
        printf("Insufficient permissions, rerun with superuser perms\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}
// Wrapper around the fopen() function. Takes into consideration
// the pretend global variable
FILE *openfile(const char *filename, const char *mode) {
  printf("\nOpening: %s\n", filename);
  if (pretend == true)
    return stdout;
  FILE *openfile = fopen(filename, mode);
  if (openfile == NULL) {
    printf("\nCan't open %s\n", filename);
    exit(EXIT_FAILURE); // or abort()
  }
  return openfile;
}
// Frees up the installation variable to avoid memory leaks
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
// Checks if the current running system is UEFI or BIOS, returns true for UEFI
// and false for BIOS
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
