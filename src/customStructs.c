typedef struct installType {
    enum privEscal { sudo, doas } privEscal;

    enum portage { binhost, source } portage;

    enum init { OpenRC, SystemD } init;

    enum desktop { noX11, Gnome, Plasma } desktop;

    // intel, i915, nvidia, radeon, amdgpu, radeonsi, virtualbox, vmware
    bool gpus[8];
    // arch, debian, fedora, ubuntu, voidlinux
    bool stratas[5];
    bool bedrock;
    bool flatpak;
    bool isUefi;
    bool kernelBin;
    bool worldUpdate;
    char *useflags;
    int makeOptJ;
    int makeOptL;
    char *timezone;
    char *filename;
    char *locales;
    char *locale;
    char *keyboard;
    char *username;
    char *hostname;
    char *userpasswd;
    char *rootpasswd;
} installType;

typedef struct part {
    char *partition;
    char *mountPoint;
    char *fileSystem;
    bool wipe;
} part;
