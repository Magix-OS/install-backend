

#include "partition_funcs.h"
#include "parse_funcs.h"
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// Unmounts all directories under /mnt/gentoo, and creates /mnt and /mnt/gentoo
// if they don't exist
void initialize_directories() {
  exec_prog_ignore_fail("umount -R /mnt/gentoo");
  DIR *dir = opendir("/mnt/gentoo");
  if (dir != NULL)
    closedir(dir);
  else {
    printf("Creating /mnt and /mnt/gentoo\n");
    if (pretend == false) {
      if ((mkdir("/mnt", 0777) != 0 || mkdir("/mnt/gentoo", 0777) != 0)) {
        printf("Missing Permissions\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}
// Formats the passed partition
void format_partition(part const part) {
  char command[COMMAND_MAX];
  if (strcmp(part.file_system, "BTRFS") == 0) {
    printf("Formatting %s as BTRFS\n", part.partition);
    sprintf(command, "mkfs.btrfs %s", part.partition);
  } else if (strcmp(part.file_system, "EXT4") == 0) {
    printf("Formatting %s as EXT4\n", part.partition);
    sprintf(command, "mkfs.ext4 %s", part.partition);
  } else if (strcmp(part.file_system, "XFS") == 0) {
    printf("Formatting %s as XFS\n", part.partition);
    sprintf(command, "mkfs.xfs %s", part.partition);
  } else if (strcmp(part.file_system, "FAT32") == 0) {
    printf("Formatting %s as FAT\n", part.partition);
    sprintf(command, "mkfs.vfat -F 32 %s", part.partition);
  } else if (strcmp(part.file_system, "SWAP") == 0) {
    printf("Formatting %s as SWAP\n", part.partition);
    sprintf(command, "mkswap %s", part.partition);
  } else {
    printf("Wrong filesystem\n");
    exit(EXIT_FAILURE);
  }
  exec_prog(command);
}
// Mount passed partition, SWAP partitions do not get mounted
void mount_partition(const part part) {
  char command[COMMAND_MAX];
  if (strcmp(part.mount_point, "SWAP") == 0) {
    sprintf(command, "swapon %s", part.partition);
  } else {
    char path[PATH_MAX];
    sprintf(path, "/mnt/gentoo%s", part.mount_point);
    DIR *dir = opendir(path);
    if (dir != NULL) {
      closedir(dir);
      sprintf(command, "umount -R %s", path);
      exec_prog_ignore_fail(command);
    } else {
      printf("Creating %s\n", path);
      if (pretend == false) {
        if (mkdir(path, 0777) != 0) {
          printf("Missing Permissions\n");
          exit(EXIT_FAILURE);
        }
      }
    }
    sprintf(command, "mount %s /mnt/gentoo%s", part.partition,
            part.mount_point);
  }
  exec_prog(command);
}
// Uses json_to_part() to parse the provided JSON for partitions, formats and
// mounts them
void prepare_partitions(json_t *root, bool const root_part, const int num) {
  for (int i = 0; i < num; i++) {
    const part partition = json_to_part(root, i);
    if (root_part == true) {
      if (strcmp(partition.mount_point, "/") == 0) {
        if (partition.wipe)
          format_partition(partition);
        else
          printf("Skipping wiping %s\n", partition.partition);
        mount_partition(partition);
      }
    } else {
      if (strcmp(partition.mount_point, "/") != 0) {
        if (partition.wipe)
          format_partition(partition);
        else
          printf("Skipping wiping %s\n", partition.partition);
        mount_partition(partition);
      }
    }

    free(partition.mount_point);
    free(partition.partition);
    free(partition.file_system);
  }
}
// Mounts necessary partitions for correct chroot operations
void mount_directories() {
  exec_prog("cp --dereference /etc/resolv.conf /mnt/gentoo/etc/");
  exec_prog("mount --types proc /proc /mnt/gentoo/proc");
  exec_prog("mount --rbind /sys /mnt/gentoo/sys");
  exec_prog("mount --rbind /dev /mnt/gentoo/dev");
  exec_prog("mount --make-rslave /mnt/gentoo/dev");
  exec_prog("mount --bind /run /mnt/gentoo/run");
  exec_prog("mount --make-slave /mnt/gentoo/run ");
}