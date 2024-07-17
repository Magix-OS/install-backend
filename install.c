#include <asm-generic/errno-base.h>
#include <curses.h>
#include <dirent.h>
#include <errno.h>
#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct part {
  char *partition;
  char *mountPoint;
  char *fileSystem;
} part;
typedef struct stage4 {
  enum init { OpenRC, SystemD } init;
  enum desktop { noX11, Gnome, Plasma } desktop;
  char *fileName;
} stage4;
typedef struct installType {
  enum privEscal { sudo, doas } privEscal;
  enum portage { binhost, source } portage;
  bool bedrock;
  enum stratas { arch, debian, fedora, ubuntu, voidlinux } stratas;
  bool flatpak;
  char *gpu;
  int makeOptJ;
  stage4 stage4;
  int makeOptL;
  bool isUefi;
  char *timezone;
  char *locale;
} installType;

// Global variables
part currentPartition;
char command[100];

// Shamelessly stolen from
// https://github.com/kernaltrap8/tinyfetch/blob/main/src/tinyfetch.c
int file_parser(const char *file, const char *line_to_read) {
  char resolved_path[PATH_MAX];
  if (realpath(file, resolved_path) == NULL) {
    perror("realpath");
    return -1;
  }

  FILE *meminfo = fopen(resolved_path, "r");
  if (meminfo == NULL) {
    perror("fopen");
    return -1;
  }

  char line[256];
  while (fgets(line, sizeof(line), meminfo)) {
    int ram;
    if (sscanf(line, line_to_read, &ram) == 1) {
      fclose(meminfo);
      return ram;
    }
  }

  fclose(meminfo);
  return -1;
}
double total_ram() {
  int total_ram = file_parser("/proc/meminfo", "MemTotal: %d kB");
  if (total_ram != -1) {
    return (double)total_ram / 1048576.0;
  } else
    exit(EXIT_FAILURE);
}

void execProg(char command[]) {
  printf("Executing %s\n", command);
  if (system(command) != 0) {
    printf("Something went wrong\n");
    if (errno == EACCES)
      printf("Insuffisant permissions, rerun with superuser perms\n");
    exit(EXIT_FAILURE);
  }
  command[0] = '\0';
}
void mountPartition(part part) {
  if (strcmp(part.mountPoint, "SWAP") == 0) {
    strcat(command, "swapon ");
    strcat(command, part.partition);
  } else {
    strcat(command, "mount ");
    strcat(command, part.partition);
    strcat(command, " /mnt/gentoo");
    strcat(command, part.mountPoint);
  }
  printf("Executing %s\n", command);
  command[0] = '\0';
  // execProg(command);
}
void formatPartition(part part) {
  char mkfsCommand[50];
  if (strcmp(part.fileSystem, "BTRFS") == 0) {
    printf("Formatting %s as BTRFS\n", part.partition);
    strcpy(mkfsCommand, "mkfs.btrfs ");
  } else if (strcmp(part.fileSystem, "EXT4") == 0) {
    printf("Formatting %s as EXT4\n", part.partition);
    strcpy(mkfsCommand, "mkfs.ext4 ");
  } else if (strcmp(part.fileSystem, "XFS") == 0) {
    printf("Formatting %s as XFS\n", part.partition);
    strcpy(mkfsCommand, "mkfs.xfs ");
  } else if (strcmp(part.fileSystem, "FAT32") == 0) {
    printf("Formatting %s as FAT\n", part.partition);
    strcpy(mkfsCommand, "mkfs.vfat -F 32 ");
  } else if (strcmp(part.fileSystem, "SWAP") == 0) {
    printf("Formatting %s as SWAP\n", part.partition);
    strcpy(mkfsCommand, "mkswap ");
  } else {
    printf("Wrong filesystem\n");
    exit(EXIT_FAILURE);
  }
  strcat(command, mkfsCommand);
  strcat(command, part.partition);
  printf("Executing %s\n", command);
  command[0] = '\0';
  // execProg(command);
}
void initializeDirectories() {
  umount2("/mnt/gentoo/efi", MNT_FORCE);
  umount2("/mnt/gentoo", MNT_FORCE);
  system("swapoff -a");
  if ((mkdir("/mnt", 0777) != 0 || mkdir("/mnt/gentoo", 0777) != 0 ||
       mkdir("/mnt/gentoo/efi", 0777) != 0) &&
      errno != EEXIST) {
    printf("Error initializing the mount points\n");
    exit(EXIT_FAILURE);
  }
}
int isUEFI() {
  DIR *dir = opendir("/sys/firmware/efi");
  if (dir) {
    closedir(dir);
    printf("UEFI system detected\n");
    return 0;
  } else if (ENOENT == errno) {
    printf("BIOS system detected\n");
    return 1;
  } else {
    printf("Errno :%d, cant detect if system is UEFI or BIOS\n", errno);
    exit(EXIT_FAILURE);
  }
}
// TODO: Add parameters to decide which one to download
void stage4DLandExtract() {
  char urlTop[] = "https://distfiles.gentoo.org/releases/amd64/autobuilds/"
                  "20240714T170402Z/";
  char urlBottom[] = "stage3-amd64-openrc-20240714T170402Z.tar.xz";
  if (chdir("/mnt/gentoo") != 0) {
    printf("Error: %d, cant change directory to /mnt/gentoo, should work, but "
           "didnt\n",
           errno);
    exit(EXIT_FAILURE);
  }
  char *command =
      malloc((strlen(urlBottom) + strlen(urlTop) + strlen("wget -v ") + 2) *
             sizeof(char));
  command[0] = '\0';
  strcat(command, "wget -v ");
  strcat(command, urlTop);
  strcat(command, urlBottom);
  execProg(command);
  char *command2 =
      malloc((strlen(urlBottom) +
              strlen("tar xpvf --xattrs-include='*.*' --numeric-owner ") + 2) *
             sizeof(char));
  strcat(command2, "tar xpvf --xattrs-include='*.*' --numeric-owner ");
  strcat(command2, urlBottom);
  execProg(command2);
}
void jsonToPart(char *path) {
  json_t *root, *partition, *data, *layout, *paths, *args, *filesystem,
      *mountpoint;
  json_error_t error;
  const char *layout_text;
  root = json_load_file(path, 0, &error);
  if (!root)
    exit(EXIT_FAILURE);
  if (!json_is_array(root)) {
    fprintf(stderr, "error: root is not an array\n");
    json_decref(root);
    exit(EXIT_FAILURE);
  }
  data = json_array_get(root, 0);
  if (!json_is_object(data)) {
    fprintf(stderr, "error : not an object\n");
    json_decref(root);
    exit(EXIT_FAILURE);
  }
  layout = json_object_get(data, "layout");
  if (!json_is_array(layout)) {
    fprintf(stderr, "error: is not a array\n");
    json_decref(root);
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < json_array_size(layout); i++) {
    partition = json_array_get(layout, i);
    if (!json_is_object(partition)) {
      fprintf(stderr, "error : not an object\n");
      json_decref(root);
      exit(EXIT_FAILURE);
    }
    args = json_object_get(partition, "args");
    paths = json_object_get(partition, "path");
    filesystem = json_object_get(args, "filesystem");
    mountpoint = json_object_get(args, "mountpoint");
    currentPartition.partition = (char *)json_string_value(paths);
    currentPartition.fileSystem = (char *)json_string_value(filesystem);
    currentPartition.mountPoint = (char *)json_string_value(mountpoint);
    formatPartition(currentPartition);
    mountPartition(currentPartition);
  }
}
int main(int argc, char *argv[]) { jsonToPart(argv[1]); }
