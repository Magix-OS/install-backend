void
initialize_directories() {
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
    if (is_uefi()) {
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
    exec_prog("swapoff -a");
}

void format_partition(part const part) {
    char command[1024];
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

int partitions_number(const char *path) {
    json_error_t error;
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

void mount_partition(const part part) {
    char command[1024];
    if (strcmp(part.mount_point, "SWAP") == 0) {
        sprintf(command, "swapon %s", part.partition);
    } else {
        sprintf(command, "mount %s /mnt/gentoo%s", part.partition, part.mount_point);
    }
    exec_prog(command);
}

void prepare_partitions(const char *path, bool const root) {
    const int part_num = partitions_number(path);
    for (int i = 0; i < part_num; i++) {
        const part partition = json_to_part(path, i);
        if (root == true) {
            if (strcmp(partition.mount_point, "/") == 0) {
                if (partition.wipe)
                    format_partition(partition);
                else
                    printf("Skipping wiping %s\n", partition.partition);
                mount_partition(partition);
                free(partition.mount_point);
                free(partition.partition);
                free(partition.file_system);
                break;
            }
        } else {
            if (partition.wipe)
                format_partition(partition);
            else
                printf("Skipping wiping %s\n", partition.partition);
            mount_partition(partition);
            free(partition.mount_point);
            free(partition.partition);
            free(partition.file_system);
        }
    }
}

void mount_directories() {
    exec_prog("cp --dereference /etc/resolv.conf /mnt/gentoo/etc/");
    exec_prog("mount --types proc /proc /mnt/gentoo/proc");
    exec_prog("mount --rbind /sys /mnt/gentoo/sys");
    exec_prog("mount --rbind /dev /mnt/gentoo/dev");
    exec_prog("mount --make-rslave /mnt/gentoo/dev");
    exec_prog("mount --bind /run /mnt/gentoo/run");
    exec_prog("mount --make-slave /mnt/gentoo/run ");
}