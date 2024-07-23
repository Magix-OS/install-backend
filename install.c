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
#define HOST                                                                   \
  "https://distfiles.gentoo.org/releases/amd64/autobuilds/20240714T170402Z/"
#define MNTROOT "/mnt/gentoo"
typedef struct part {
  char *partition;
  char *mountPoint;
  char *fileSystem;
  bool wipe;
} part;
typedef struct stage4 {
  enum init { OpenRC, SystemD } init;
  enum desktop { noX11, Gnome, Plasma } desktop;
  char *fileName;
} stage4;
typedef struct installType {
  enum privEscal { sudo, doas } privEscal;
  enum portage { binhost, source } portage;
  enum stratas { arch, debian, fedora, ubuntu, voidlinux } stratas[5];
  stage4 stage4;
  bool gpus[8];
  int makeOptJ;
  int makeOptL;
  bool bedrock;
  bool flatpak;
  bool isUefi;
  char *timezone;
  char *locales;
  char *locale;
  char *keyboard;
  char *username;
  char *hostname;
  char *userpasswd;
  char *rootpasswd;
} installType;

// Global variables
char command[100];
// Shamelessly stolen from
// https://github.com/kernaltrap8/tinyfetch/blob/main/src/tinyfetch.c
bool isUEFI() {
  DIR *dir = opendir("/sys/firmware/efi");
  if (dir) {
    closedir(dir);
    return true;
  } else if (ENOENT == errno)
    return false;
  else {
    printf("Errno :%d, cant detect if system is UEFI or BIOS\n", errno);
    exit(EXIT_FAILURE);
  }
}
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
    strcat(command, " ");
    strcat(command, MNTROOT);
    strcat(command, part.mountPoint);
  }
  execProg(command);
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
  execProg(command);
}
void initializeDirectories() {
  if (opendir("/mnt/gentoo") != NULL)
    umount2("/mnt/gentoo", MNT_FORCE);
  else {
    if ((mkdir("/mnt", 0777) != 0 || mkdir("/mnt/gentoo", 0777) != 0)) {
      printf("Missing Permissions\n");
      exit(EXIT_FAILURE);
    }
  }
  if (isUEFI()) {
    if (opendir("/mnt/gentoo/efi") != NULL)
      umount2("/mnt/gentoo/efi", MNT_FORCE);
    else {
      if ((mkdir("/mnt/gentoo/efi", 0777) != 0)) {
        printf("Missing Permissions, cant create /mnt/gentoo/efi\n");
        exit(EXIT_FAILURE);
      }
    }
  }
  system("swapoff -a");
}

// TODO: Add parameters to decide which one to download
void stage4DLandExtract() {
  char urlBottom[] = "stage3-amd64-openrc-20240714T170402Z.tar.xz";
  if (chdir("/mnt/gentoo") != 0) {
    printf("Error: %d, cant change directory to /mnt/gentoo, should work, but "
           "didnt\n",
           errno);
    exit(EXIT_FAILURE);
  }
  strcat(command, "wget -v ");
  strcat(command, HOST);
  strcat(command, urlBottom);
  execProg(command);
  strcpy(command,
         "tar xpvf stage3-*.tar.xz --xattrs-include='*.*' --numeric-owner");
  execProg(command);
}
installType jsonToConf(char *path) {
  json_t *gpus, *stratas, *locales, *config, *root, *data;

  installType install;
  json_error_t error;
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
  config = json_object_get(data, "config");
  if (!json_is_object(config)) {
    fprintf(stderr, "error: is not a array\n");
    json_decref(root);
    exit(EXIT_FAILURE);
  }
  install.isUefi = isUEFI();
  install.privEscal = json_boolean_value(json_object_get(config, "privEscal"));
  install.portage = json_boolean_value(json_object_get(config, "portage"));
  install.stage4.init = json_boolean_value(json_object_get(config, "init"));
  install.stage4.desktop =
      json_integer_value(json_object_get(config, "desktop"));
  install.makeOptJ = json_integer_value(json_object_get(config, "makeOptJ"));
  install.makeOptL = json_integer_value(json_object_get(config, "makeOptL"));
  install.bedrock = json_boolean_value(json_object_get(config, "bedrock"));
  install.flatpak = json_boolean_value(json_object_get(config, "flatpak"));
  install.timezone = (char *)malloc(
      sizeof(char) *
      (1 + strlen(json_string_value(json_object_get(config, "timezone")))));
  strcpy(install.timezone,
         json_string_value(json_object_get(config, "timezone")));
  install.locale = (char *)malloc(
      sizeof(char) *
      (1 + strlen(json_string_value(json_object_get(config, "locale")))));
  strcpy(install.locale, json_string_value(json_object_get(config, "locale")));
  install.keyboard = (char *)malloc(
      sizeof(char) *
      (1 + strlen(json_string_value(json_object_get(config, "keyboard")))));
  strcpy(install.keyboard,
         json_string_value(json_object_get(config, "keyboard")));
  install.username = (char *)malloc(
      sizeof(char) *
      (1 + strlen(json_string_value(json_object_get(config, "username")))));
  strcpy(install.username,
         json_string_value(json_object_get(config, "username")));
  install.hostname = (char *)malloc(
      sizeof(char) *
      (1 + strlen(json_string_value(json_object_get(config, "hostname")))));
  strcpy(install.hostname,
         json_string_value(json_object_get(config, "hostname")));
  install.userpasswd = (char *)malloc(
      sizeof(char) *
      (1 + strlen(json_string_value(json_object_get(config, "passwd")))));
  strcpy(install.userpasswd,
         json_string_value(json_object_get(config, "passwd")));
  install.rootpasswd = (char *)malloc(
      sizeof(char) *
      (1 + strlen(json_string_value(json_object_get(config, "rootpasswd")))));
  strcpy(install.rootpasswd,
         json_string_value(json_object_get(config, "rootpasswd")));
  locales = json_object_get(config, "locales");
  if (!json_is_array(locales)) {
    fprintf(stderr, "error: is not a array\n");
    json_decref(root);
    exit(EXIT_FAILURE);
  }
  int localeLength = 0;
  for (int i = 0; i < json_array_size(locales); i++)
    localeLength = localeLength + 1 +
                   strlen(json_string_value(json_array_get(locales, i)));

  install.locales = (char *)malloc(sizeof(char) * (1 + localeLength));
  sprintf(install.locales, "%s\n",
          json_string_value(json_array_get(locales, 0)));
  for (int i = 1; i < json_array_size(locales); i++) {
    strcat(install.locales, json_string_value(json_array_get(locales, i)));
    strcat(install.locales, "\n\0");
  }
  stratas = json_object_get(config, "stratas");
  if (!json_is_array(stratas)) {
    fprintf(stderr, "error: is not a array\n");
    json_decref(root);
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < json_array_size(stratas); i++)
    install.stratas[i] = json_boolean_value(json_array_get(stratas, i));

  gpus = json_object_get(config, "gpus");
  if (!json_is_array(gpus)) {
    fprintf(stderr, "error: is not a array\n");
    json_decref(root);
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < json_array_size(gpus); i++)
    install.gpus[i] = json_boolean_value(json_array_get(gpus, i));
  json_decref(root);
  return install;
}
part jsonToPart(char *path, int i) {
  json_t *root, *partition, *data, *layout, *paths, *args, *filesystem,
      *mountpoint;
  json_error_t error;
  part part;
  bool wipe;
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
  wipe = json_boolean_value(json_object_get(args, "wipe"));
  part.partition =
      (char *)malloc(sizeof(char) * (1 + strlen(json_string_value(paths))));
  strcpy(part.partition, json_string_value(paths));
  part.fileSystem = (char *)malloc(sizeof(char) *
                                   (1 + strlen(json_string_value(filesystem))));
  strcpy(part.fileSystem, json_string_value(filesystem));
  part.mountPoint = (char *)malloc(sizeof(char) *
                                   (1 + strlen(json_string_value(mountpoint))));
  strcpy(part.mountPoint, json_string_value(mountpoint));
  part.wipe = wipe;
  json_decref(root);
  return part;
}
int partitionsNumber(char *path) {
  json_t *root, *data, *layout;
  json_error_t error;
  bool wipe;
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
  int num = json_array_size(layout);
  json_decref(root);
  return num;
}
void freeInstall(installType install) {
  free(install.hostname);
  free(install.keyboard);
  free(install.locale);
  free(install.locales);
  free(install.rootpasswd);
  free(install.username);
  free(install.userpasswd);
  free(install.timezone);
}
void freePart(part part) {

  free(part.mountPoint);
  free(part.partition);
  free(part.fileSystem);
}
void chrootPrepare() {

  strcpy(command, "cp --dereference /etc/resolv.conf /mnt/gentoo/etc/");
  execProg(command);
  strcpy(command, "mount --types proc /proc /mnt/gentoo/proc");
  execProg(command);
  strcpy(command, "mount --rbind /sys /mnt/gentoo/sys");
  execProg(command);
  strcpy(command, "mount --rbind /dev /mnt/gentoo/dev");
  execProg(command);
  strcpy(command, "mount --make-rslave /mnt/gentoo/dev");
  execProg(command);
  strcpy(command, "mount --bind /run /mnt/gentoo/run");
  execProg(command);
  strcpy(command, "mount --make-slave /mnt/gentoo/run ");
  execProg(command);
}
void chrootUnprepare() {
  strcpy(command, "umount -R *");
  execProg(command);
}
int main(int argc, char *argv[]) {
  part partition;
  int partNum = partitionsNumber(argv[1]);
  initializeDirectories();
  for (int i = 0; i < partNum; i++) {
    partition = jsonToPart(argv[1], i);
    if (partition.wipe)
      formatPartition(partition);
    else
      printf("Skipping wiping %s\n", partition.partition);
    mountPartition(partition);
    freePart(partition);
  }
  installType install = jsonToConf(argv[1]);
  stage4DLandExtract();
  chrootPrepare();
  // TODO:Add chroot operations
  chrootUnprepare();
  freeInstall(install);
  return 0;
}
