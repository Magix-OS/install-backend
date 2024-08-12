void initializeDirectories() {
    if (opendir("/mnt/gentoo") != NULL)
        umount2("/mnt/gentoo", MNT_FORCE);
    else {
        printf("Creating /mnt and /mnt/gentoo\n");
        if (pretend == 0) {
            if ((mkdir("/mnt", 0777) != 0 || mkdir("/mnt/gentoo", 0777) != 0)) {
                printf("Missing Permissions\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    if (isUEFI()) {
        if (opendir("/mnt/gentoo/efi") != NULL)
            umount2("/mnt/gentoo/efi", MNT_FORCE);
        else {
            printf("Creating /mnt/gentoo/efi\n");
            if (pretend == 0) {
                if ((mkdir("/mnt/gentoo/efi", 0777) != 0)) {
                    printf("Missing Permissions, cant create /mnt/gentoo/efi\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
    } else {
        if (opendir("/mnt/gentoo/boot") != NULL)
            umount2("/mnt/gentoo/boot", MNT_FORCE);
        else {
            printf("Creating /mnt/gentoo/boot\n");
            if (pretend == 0) {
                if ((mkdir("/mnt/gentoo/boot", 0777) != 0)) {
                    printf("Missing Permissions, cant create /mnt/gentoo/boot\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    execProg("swapoff -a");
}
void formatPartition(part part) {
    char command[1024];
    if (strcmp(part.fileSystem, "BTRFS") == 0) {
        printf("Formatting %s as BTRFS\n", part.partition);
        sprintf(command, "mkfs.btrfs %s", part.partition);
    } else if (strcmp(part.fileSystem, "EXT4") == 0) {
        printf("Formatting %s as EXT4\n", part.partition);
        sprintf(command, "mkfs.ext4 %s", part.partition);
    } else if (strcmp(part.fileSystem, "XFS") == 0) {
        printf("Formatting %s as XFS\n", part.partition);
        sprintf(command, "mkfs.xfs %s", part.partition);
    } else if (strcmp(part.fileSystem, "FAT32") == 0) {
        printf("Formatting %s as FAT\n", part.partition);
        sprintf(command, "mkfs.vfat -F 32 %s", part.partition);
    } else if (strcmp(part.fileSystem, "SWAP") == 0) {
        printf("Formatting %s as SWAP\n", part.partition);
        sprintf(command, "mkswap %s", part.partition);
    } else {
        printf("Wrong filesystem\n");
        exit(EXIT_FAILURE);
    }
    execProg(command);
}
int partitionsNumber(const char *path) {
    json_error_t error;
    bool wipe;
    json_t *root = json_load_file(path, 0, &error);
    if (root == NULL)
        exit(EXIT_FAILURE);
    if (!json_is_array(root)) {
        fprintf(stderr, "error: root is not an array\n");
        json_decref(root);
        exit(EXIT_FAILURE);
    }
    const json_t *data = json_array_get(root, 0);
    if (!json_is_object(data)) {
        fprintf(stderr, "error : not an object\n");
        json_decref(root);
        exit(EXIT_FAILURE);
    }
    const json_t *layout = json_object_get(data, "layout");
    if (!json_is_array(layout)) {
        fprintf(stderr, "error: is not a array\n");
        json_decref(root);
        exit(EXIT_FAILURE);
    }
    const int num = json_array_size(layout);
    json_decref(root);
    return num;
}
void mountPartition(const part part) {
    char command[1024];
    if (strcmp(part.mountPoint, "SWAP") == 0) {
        sprintf(command, "swapon %s", part.partition);
    } else {
        sprintf(command, "mount %s /mnt/gentoo%s", part.partition, part.mountPoint);
    }
    execProg(command);
}
void preparePartitions(const char *path, bool root) {
    const int partNum = partitionsNumber(path);
    for (int i = 0; i < partNum; i++) {
        const part partition = jsonToPart(path, i);
        if( root == true) {
            if(strcmp(partition.mountPoint,"/") == 0) {
                if (partition.wipe)
                    formatPartition(partition);
                else
                    printf("Skipping wiping %s\n", partition.partition);
                mountPartition(partition);
                free(partition.mountPoint);
                free(partition.partition);
                free(partition.fileSystem);
                break;
            }
        }
        else {
            if (partition.wipe)
                formatPartition(partition);
            else
                printf("Skipping wiping %s\n", partition.partition);
            mountPartition(partition);
            free(partition.mountPoint);
            free(partition.partition);
            free(partition.fileSystem);
        }
    }
}

void mountDirectories() {
    execProg("cp --dereference /etc/resolv.conf /mnt/gentoo/etc/");
    execProg("mount --types proc /proc /mnt/gentoo/proc");
    execProg("mount --rbind /sys /mnt/gentoo/sys");
    execProg("mount --rbind /dev /mnt/gentoo/dev");
    execProg("mount --make-rslave /mnt/gentoo/dev");
    execProg("mount --bind /run /mnt/gentoo/run");
    execProg("mount --make-slave /mnt/gentoo/run ");
}
