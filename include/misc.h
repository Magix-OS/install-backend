//
// Created by crystal on 14/09/24.
//

#ifndef MISC_H
#define MISC_H
#include <stdbool.h>
#include <stdio.h>
extern bool pretend;
extern const char *stratas[];
extern const char *filesystems[];
typedef struct install_type {
  // arch, debian, fedora, ubuntu, void linux
  bool stratas[5];
  // xfs, ext4, vfat, btrfs, zfs, jfs
  bool filesystems[6];
  bool systemd;
  bool use_doas;
  bool binhost;
  bool bedrock;
  bool flatpak;
  bool is_uefi;
  bool kernel_bin;
  bool world_update;
  bool intel_microcode;
  bool sof_firmware;
  bool linux_firmware;
  int make_opt_j;
  int make_opt_l;
  int partitions_number;
  char *useflags;
  char *gpus;
  char *grub_disk;
  char *timezone;
  char *filename;
  char *locales;
  char *locale;
  char *keyboard;
  char *username;
  char *hostname;
  char *userpasswd;
  char *rootpasswd;
  char *packages;
} install_type;

typedef struct part {
  char *partition;
  char *mount_point;
  char *file_system;
  bool wipe;
} part;
void exec_prog(char *command);
void exec_prog_ignore_fail(char *command);
FILE *openfile(const char *filename, const char *mode);
void free_install(const install_type current);
bool is_uefi();

#endif //MISC_H
