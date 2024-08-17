const char *cards[] = {
    "intel", "i915", "nvidia", "radeon",
    "amdgpu", "radeonsi", "virtualbox", "vmware"
};
const char *stratas[] = {"arch", "debian", "fedora", "ubuntu", "voidlinux"};
const char *filesystems[] = {
    "sys-fs/xfsprogs", "sys-fs/e2fsprogs", "sys-fs/dosfstools", "sys-fs/btrfs-progs", "sys-fs/zfs", "sys-fs/jfsutils"
};

void output_details(install_type const current) {
    printf(
        "Useflags: %s\nTimezone: %s\nFilename: %s\nLocales: %s\nPrimary Locale: %s\nKeyboard Layout: %s\nUsername: %s\nHostname: %s\nUserpassword: %s\nRootpassword: %s\nMakeopts: %d %d\nCards:",
        current.useflags, current.timezone, current.filename, current.locales, current.locale, current.keyboard,
        current.username,
        current.hostname, current.userpasswd, current.rootpasswd, current.make_opt_j, current.make_opt_l);
    for (int i = 0; i < 8; i++) {
        if (current.gpus[i]) {
            printf(" %s", cards[i]);
        }
    }
    if (current.bedrock)
        printf("\nBedrock: Yes");
    else
        printf("\nBedrock: No");
    if (current.flatpak)
        printf("\nFlatpak: Yes");
    else
        printf("\nFlatpak: No");
    if (current.kernel_bin)
        printf("\nBinary Kernel: Yes");
    else
        printf("\nBinary Kernel: No");
    printf("\nStratas:");
    for (int i = 0; i < 5; i++) {
        if (current.stratas[i]) {
            printf(" %s", stratas[i]);
        }
    }
    printf("\nFilesystems:");
    for (int i = 0; i < 6; i++) {
        if (current.filesystems[i]) {
            printf(" %s", filesystems[i]);
        }
    }
    if (current.priv_escal == sudo)
        printf("\nPriv tool: sudo");
    else
        printf("\nPriv tool: doas");
    if (current.portage == binhost)
        printf("\nPortage packages: binary");
    else
        printf("\nPortage packages: source");
    if (current.init == open_rc)
        printf("\nInit: OpenRC");
    else
        printf("\nInit: Systemd");
    if (current.desktop == no_x11)
        printf("\nDesktop: None");
    else if (current.desktop == gnome)
        printf("\nDesktop: Gnome");
    else
        printf("\nDesktop: Plasma");
    if (current.world_update)
        printf("\nWorld Update: Yes");
    else
        printf("\nWorld Update: No");
    if (current.sof_firmware)
        printf("\nSof firmware: Yes");
    else
        printf("\nSof firmware: No");
    if (current.intel_microcode)
        printf("\nIntel microcodes: Yes");
    else
        printf("\nIntel microcodes: No");
    if (current.linux_firmware)
        printf("\nLinux Firmware: Yes");
    else
        printf("\nLinux Firmware: No");
}

bool is_uefi() {
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

install_type json_to_conf(const char *path) {
    json_error_t error;
    json_t *root = json_load_file(path, 0, &error);
    install_type install;
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
    const json_t *config = json_object_get(data, "config");
    if (!json_is_object(config)) {
        fprintf(stderr, "error: is not a array\n");
        json_decref(root);
        exit(EXIT_FAILURE);
    }
    install.is_uefi = is_uefi();
    install.priv_escal = json_boolean_value(json_object_get(config, "priv_escal"));
    install.kernel_bin = json_boolean_value(json_object_get(config, "kernel"));
    install.portage = json_boolean_value(json_object_get(config, "portage"));
    install.init = json_boolean_value(json_object_get(config, "init"));
    install.world_update = json_boolean_value(json_object_get(config, "world_update"));
    install.desktop =
            json_integer_value(json_object_get(config, "desktop"));
    install.make_opt_j = json_integer_value(json_object_get(config, "make_opt_j"));
    install.make_opt_l = json_integer_value(json_object_get(config, "make_opt_l"));
    install.bedrock = json_boolean_value(json_object_get(config, "bedrock"));
    install.flatpak = json_boolean_value(json_object_get(config, "flatpak"));
    install.intel_microcode = json_boolean_value(json_object_get(config, "intel_microcode"));
    install.sof_firmware = json_boolean_value(json_object_get(config, "sof_microcode"));
    install.linux_firmware = json_boolean_value(json_object_get(config, "linux_firmware"));
    install.timezone = (char *) malloc(
        sizeof(char) *
        (1 + strlen(json_string_value(json_object_get(config, "timezone")))));
    strcpy(install.timezone,
           json_string_value(json_object_get(config, "timezone")));

    install.useflags = (char *) malloc(
        sizeof(char) *
        (1 + strlen(json_string_value(json_object_get(config, "useflags")))));
    strcpy(install.useflags,
           json_string_value(json_object_get(config, "useflags")));
    install.locale = (char *) malloc(
        sizeof(char) *
        (1 + strlen(json_string_value(json_object_get(config, "locale")))));
    strcpy(install.locale, json_string_value(json_object_get(config, "locale")));
    install.keyboard = (char *) malloc(
        sizeof(char) *
        (1 + strlen(json_string_value(json_object_get(config, "keyboard")))));
    strcpy(install.keyboard,
           json_string_value(json_object_get(config, "keyboard")));
    install.username = (char *) malloc(
        sizeof(char) *
        (1 + strlen(json_string_value(json_object_get(config, "username")))));
    strcpy(install.username,
           json_string_value(json_object_get(config, "username")));
    install.hostname = (char *) malloc(
        sizeof(char) *
        (1 + strlen(json_string_value(json_object_get(config, "hostname")))));
    strcpy(install.hostname,
           json_string_value(json_object_get(config, "hostname")));
    install.filename = (char *) malloc(
        sizeof(char) *
        (1 + strlen(json_string_value(json_object_get(config, "filename")))));
    strcpy(install.filename,
           json_string_value(json_object_get(config, "filename")));
    install.userpasswd = (char *) malloc(
        sizeof(char) *
        (1 + strlen(json_string_value(json_object_get(config, "passwd")))));
    strcpy(install.userpasswd,
           json_string_value(json_object_get(config, "passwd")));
    install.rootpasswd = (char *) malloc(
        sizeof(char) *
        (1 + strlen(json_string_value(json_object_get(config, "rootpasswd")))));
    strcpy(install.rootpasswd,
           json_string_value(json_object_get(config, "rootpasswd")));
    const json_t *locales = json_object_get(config, "locales");
    if (!json_is_array(locales)) {
        fprintf(stderr, "error: is not a array\n");
        json_decref(root);
        exit(EXIT_FAILURE);
    }
    int locale_length = 0;
    for (int i = 0; i < json_array_size(locales); i++)
        locale_length = locale_length + 1 +
                        strlen(json_string_value(json_array_get(locales, i)));

    install.locales = (char *) malloc(sizeof(char) * (1 + locale_length));
    sprintf(install.locales, "%s\n",
            json_string_value(json_array_get(locales, 0)));
    for (int i = 1; i < json_array_size(locales); i++) {
        strcat(install.locales, json_string_value(json_array_get(locales, i)));
        if (i != json_array_size(locales) - 1)
            strcat(install.locales, "\n");
    }
    const json_t *stratas = json_object_get(config, "stratas");
    if (!json_is_array(stratas)) {
        fprintf(stderr, "error: is not a array\n");
        json_decref(root);
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < json_array_size(stratas); i++)
        install.stratas[i] = json_boolean_value(json_array_get(stratas, i));

    const json_t *gpus = json_object_get(config, "gpus");
    if (!json_is_array(gpus)) {
        fprintf(stderr, "error: is not a array\n");
        json_decref(root);
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < json_array_size(gpus); i++)
        install.gpus[i] = json_boolean_value(json_array_get(gpus, i));
    const json_t *filesystems = json_object_get(config, "filesystems");
    if (!json_is_array(filesystems)) {
        fprintf(stderr, "error: is not a array\n");
        json_decref(root);
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < json_array_size(filesystems); i++)
        install.filesystems[i] = json_boolean_value(json_array_get(filesystems, i));
    json_decref(root);
    return install;
}

part json_to_part(const char *path, const int i) {
    json_error_t error;
    part part;
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
    const json_t *partition = json_array_get(layout, i);
    if (!json_is_object(partition)) {
        fprintf(stderr, "error : not an object\n");
        json_decref(root);
        exit(EXIT_FAILURE);
    }
    const json_t *args = json_object_get(partition, "args");
    const json_t *paths = json_object_get(partition, "path");
    const json_t *filesystem = json_object_get(args, "filesystem");
    const json_t *mountpoint = json_object_get(args, "mountpoint");
    const bool wipe = json_boolean_value(json_object_get(args, "wipe"));
    part.partition =
            (char *) malloc(sizeof(char) * (1 + strlen(json_string_value(paths))));
    strcpy(part.partition, json_string_value(paths));
    part.file_system = (char *) malloc(sizeof(char) *
                                       (1 + strlen(json_string_value(filesystem))));
    strcpy(part.file_system, json_string_value(filesystem));
    part.mount_point = (char *) malloc(sizeof(char) *
                                       (1 + strlen(json_string_value(mountpoint))));
    strcpy(part.mount_point, json_string_value(mountpoint));
    part.wipe = wipe;
    json_decref(root);
    return part;
}
