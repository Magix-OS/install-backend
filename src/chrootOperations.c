

void extract_chroot(install_type const install) {
    char command[1024];
    char path[256] = {0};
    if (getcwd(path, sizeof(path)) == NULL) {
        perror("getcwd");
        exit(EXIT_FAILURE); // or abort()
    }
    chdir("/mnt/gentoo");
    sprintf(command, "cp %s .", install.filename);
    exec_prog(command);
    sprintf(command, "tar xpvf %s --xattrs-include='*.*' --numeric-owner", install.filename);
    exec_prog(command);
    chdir(path);
    FILE *makeconf = openfile("/mnt/gentoo/etc/portage/make.conf", "a");
    fprintf(makeconf, "MAKEOPTS=\"-j%d -l%d\"\n", install.make_opt_j, install.make_opt_l);
    fprintf(makeconf, "USE=\"${USE} %s networkmanager\"\n", install.useflags);
    fprintf(makeconf, "ACCEPT_LICENSE=\"*\"\n");
    fprintf(makeconf, "VIDEO_CARDS=\"");
    for (int i = 0; i < 8; i++) {
        if (install.gpus[i]) {
            fprintf(makeconf, "%s ", cards[i]);
        }
    }
    fprintf(makeconf, "\"\n");
    fprintf(makeconf, "GRUB_PLATFORMS=\"efi-64\"\n");
    if (install.portage == false) {
        fprintf(makeconf,
                R"(# Appending getbinpkg to the list of values within the FEATURES variable
FEATURES="${FEATURES} getbinpkg"
# Require signatures
FEATURES="${FEATURES} binpkg-request-signature"
)");
        FILE *gentoobinhost = openfile("/mnt/gentoo/etc/portage/binrepos.conf/gentoobinhost.conf", "a");
        fprintf(gentoobinhost, "priority = 9999");
        if (pretend == 0)
            fclose(gentoobinhost);
    }
    if (pretend == 0)
        fclose(makeconf);

    if (install.init == open_rc) {
        FILE *timezone = openfile("/mnt/gentoo/etc/timezone", "w");
        fprintf(timezone, "%s", install.timezone);
        if (pretend == 0)
            fclose(timezone);
    }

    FILE *localegen = openfile("/mnt/gentoo/etc/locale.gen", "w");;
    fprintf(localegen, "%s", install.locales);
    if (pretend == 0)
        fclose(localegen);
    FILE *localeconf;
    if (install.init == open_rc)
        localeconf = openfile("/mnt/gentoo/etc/env.d/02locale", "w");
    else
        localeconf = openfile("/mnt/gentoo/etc/locale.conf", "w");

    fprintf(localeconf, "LANG=\"%s\"\nLC_COLLATE=\"C.UTF-8\"", install.locale);
    if (pretend == 0)
        fclose(localeconf);

    FILE *installkernel = openfile("/mnt/gentoo/etc/portage/package.use/installkernel", "w");
    fprintf(installkernel, "sys-kernel/installkernel grub dracut");
    if (pretend == 0)
        fclose(installkernel);
    FILE *hostname = openfile("/mnt/gentoo/etc/hostname", "w");
    fprintf(hostname, "%s", install.hostname);
    if (pretend == 0)
        fclose(hostname);
    FILE *hosts = openfile("/mnt/gentoo/etc/hosts", "w");
    fprintf(hosts, "127.0.0.1     %s.homenetwork %s localhost", install.hostname, install.hostname);
    if (pretend == 0)
        fclose(hosts);
    FILE *keymaps;
    if (install.init == open_rc) {
        keymaps = openfile("/mnt/gentoo/etc/conf.d/keymaps", "w");
        fprintf(keymaps, "keymap=\"%s\"\nwindowkeys=\"YES\"\nfix_euro=\"YES\"", install.keyboard);
    } else {
        keymaps = openfile("/mnt/gentoo/etc/vconsole.conf", "w");
        fprintf(keymaps, "KEYMAP=\"%s\"", install.keyboard);
    }
    if (pretend == 0)
        fclose(keymaps);

    if (install.priv_escal == doas) {
        FILE *sudoas = openfile("/mnt/gentoo/etc/doas.conf", "w");
        fprintf(sudoas, "permit :wheel");
        if (pretend == 0)
            fclose(keymaps);
    }
}

void mk_script(install_type const install) {
    FILE *script = openfile("/mnt/gentoo/script.sh", "w+");
    chmod("/mnt/gentoo/script.sh",S_IXOTH);
    fprintf(script,
            "#!/bin/bash\nset -e\nsource /etc/profile\nemerge-webrsync\nemerge --sync\n");
    if (install.portage == false)
        fprintf(script, "getuto\n");
    fprintf(script, "echo \"*/* $(cpuid2cpuflags)\" > /etc/portage/package.use/00cpu-flags\n");
    if (install.world_update)
        fprintf(script, "emerge --verbose --update --deep --newuse @world\n");
    if (install.init == system_d) {
        fprintf(script, "ln -sf ../usr/share/zoneinfo/%s /etc/localtime\n", install.timezone);
    }
    fprintf(script, "locale-gen\nenv-update && source /etc/profile\n");
    if (install.linux_firmware)
        fprintf(script, "emerge -v sys-kernel/linux-firmware\n");
    if (install.sof_firmware)
        fprintf(script, "emerge -v sys-firmware/sof-firmware\n");
    if (install.intel_microcode)
        fprintf(script, "emerge -v sys-firmware/intel-microcode\n");
    if (install.kernel_bin)
        fprintf(script, "emerge -v sys-kernel/gentoo-kernel-bin\n");
    else
        fprintf(script, "emerge -v sys-kernel/gentoo-kernel\n");

    fprintf(script, "emerge -v sys-kernel/dracut\n");
    fprintf(script, "emerge -v genfstab\ngenfstab / > /etc/fstab\n");
    fprintf(script, "echo -e \"%s\n%s\" | passwd -q\n", install.rootpasswd, install.rootpasswd);
    if (install.init == system_d) {
        fprintf(script, "systemd-machine-id-setup\nsystemctl preset-all --preset-mode=enable-only\n");
    }
    fprintf(script, "emerge -v");
    for (int i = 0; i < 6; i++) {
        if (install.filesystems[i]) {
            fprintf(script, " %s", filesystems[i]);
        }
    }
    fprintf(script, " sys-block/io-scheduler-udev-rules\n");
    fprintf(script, "emerge -vsys-boot/grub\n");
    if (install.is_uefi)
        fprintf(script, "grub-install --efi-directory=/efi\n");
    else
        fprintf(script, "grub-install %s\n", install.grub_disk);

    fprintf(script, "grub-mkconfig -o /boot/grub/grub.cfg\n");
    fprintf(script, "useradd -m -G users,wheel,audio,plugdev,video,input -s /bin/bash %s\n", install.username);
    fprintf(script, "echo -e \"%s\n%s\" | passwd -q %s\n", install.userpasswd, install.userpasswd, install.username);
    if (install.priv_escal == doas)
        fprintf(script,
                "emerge -v app-admin/doas\nemerge -C sudo\nchown -c root:root /etc/doas.conf\nchmod -c 0400 /etc/doas.conf\n");
    fprintf(script, "exit");

    fclose(script);
}

void exec_chroot() {
    char path[256] = {0};
    if (getcwd(path, sizeof(path)) == NULL) {
        perror("getcwd");
        exit(EXIT_FAILURE); // or abort()
    }
    printf("\nYou are here :%s\n", path);
    chdir("/mnt/gentoo");
    chroot("/mnt/gentoo");
    exec_prog("./script.sh");
    chdir(path);
}

void clean_up() {
    exec_prog("umount -l /mnt/gentoo/dev{/shm,/pts,} ");
    exec_prog("umount -R /mnt/gentoo/ ");
}
