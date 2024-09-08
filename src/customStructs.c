typedef struct install_type {
    enum priv_escal { sudo, doas } priv_escal;

    enum portage { binhost, source } portage;

    enum init { open_rc, system_d } init;

    // arch, debian, fedora, ubuntu, voidlinux
    bool stratas[5];
    // xfs, ext4, vfat, btrfs, zfs, jfs
    bool filesystems[6];
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
