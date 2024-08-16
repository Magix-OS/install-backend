#include <unistd.h>

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
    fprintf(makeconf, "USE=\"${USE} %s\"\n", install.useflags);
    fprintf(makeconf, "ACCEPT_LICENSE=\"*\"\n");
    fprintf(makeconf, "VIDEO_CARDS=\"");
    for (int i = 0; i < 8; i++) {
        if (install.gpus[i]) {
            fprintf(makeconf, "%s ", cards[i]);
        }
    }
    fprintf(makeconf, "\"\n");
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
}

void mk_script(install_type const install) {
    char path[256] = {0};
    if (getcwd(path, sizeof(path)) == NULL) {
        perror("getcwd");
        exit(EXIT_FAILURE); // or abort()
    }
    printf("\n%s\n", path);
    FILE *script = openfile("/mnt/gentoo/script.sh", "w+");
    chmod("script.sh",S_IXOTH);
    fprintf(script, "#!/bin/bash\nset -e\nsource /etc/profile\nemerge-webrsync\nemerge --sync\n");
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
}
