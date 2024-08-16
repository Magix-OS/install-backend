typedef struct install_type {
    enum priv_escal { sudo, doas } priv_escal;

    enum portage { binhost, source } portage;

    enum init { open_rc, system_d } init;

    enum desktop { no_x11, gnome, plasma } desktop;

    // intel, i915, nvidia, radeon, amdgpu, radeonsi, virtualbox, vmware
    bool gpus[8];
    // arch, debian, fedora, ubuntu, voidlinux
    bool stratas[5];
    bool bedrock;
    bool flatpak;
    bool is_uefi;
    bool kernel_bin;
    bool world_update;
    bool intel_microcode;
    bool sof_firmware;
    bool linux_firmware;
    char *useflags;
    int make_opt_j;
    int make_opt_l;
    char *timezone;
    char *filename;
    char *locales;
    char *locale;
    char *keyboard;
    char *username;
    char *hostname;
    char *userpasswd;
    char *rootpasswd;
} install_type;

typedef struct part {
    char *partition;
    char *mount_point;
    char *file_system;
    bool wipe;
} part;
