#include <unistd.h>

void extract_chroot(install_type const install) {
    char command[1024];
    chdir("/mnt/gentoo");
    sprintf(command, "cp %s .", install.filename);
    exec_prog(command);
    sprintf(command, "tar xpvf %s --xattrs-include='*.*' --numeric-owner", install.filename);
    exec_prog(command);
    printf("Editing /etc/portage/make.conf\n");
    FILE *makeconf;
    if (pretend == 1)
        makeconf = stdout;
    else
        makeconf = fopen("etc/portage/make.conf", "a");
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
        FILE *gentoobinhost;
        printf("\nEditing /etc/portage/binrepos.conf/gentoobinhost.conf\n");
        if (pretend == 1)
            gentoobinhost = stdout;
        else
            gentoobinhost = fopen("etc/portage/binrepos.conf/gentoobinhost.conf", "a");
        fprintf(gentoobinhost, "priority = 9999");
        if (pretend == 0)
            fclose(gentoobinhost);
    }
    if (pretend == 0)
        fclose(makeconf);

    if (install.init == open_rc) {
        FILE *timezone;
        if (pretend == 1)
            timezone = stdout;
        else
            timezone = fopen("/mnt/gentoo/etc/timezone", "w");
        printf("\nEditing /etc/timezone\n");
        fprintf(timezone, "%s", install.timezone);
        if (pretend == 0)
            fclose(timezone);
    }

    FILE *localegen;
    if (pretend == 1)
        localegen = stdout;
    else
        localegen = fopen("/mnt/gentoo/etc/locale.gen", "w");
    printf("\nEditing /etc/locale.gen\n");
    fprintf(localegen, "%s", install.locales);
    if (pretend == 0)
        fclose(localegen);
    FILE *localeconf;
    if (pretend == 1)
        localeconf = stdout;
    else if (install.init == open_rc)
        localeconf = fopen("/mnt/gentoo/etc/env.d/02locale", "w");
    else
        localeconf = fopen("/mnt/gentoo/etc/locale.conf", "w");

    printf("\nEditing either /etc/locale.conf or /etc/env.d/02locale\n");
    fprintf(localeconf, "LANG=\"%s\"\nLC_COLLATE=\"C.UTF-8\"", install.locale);
    if (pretend == 0)
        fclose(localeconf);
}

void mk_script(install_type const install) {
    char path[256] = {0};
    if (getcwd(path, sizeof(path)) == NULL) {
        perror("getcwd");
        exit(EXIT_FAILURE); // or abort()
    }
    printf("\n%s\n", path);
    printf("Editing script.sh\n");
    FILE *script;
    if (pretend == 1)
        script = stdout;
    else
        script = fopen("script.sh", "w+");
    chmod("script.sh",S_IXOTH);
    fprintf(script, "#!/bin/bash\nset -e\nsource /etc/profile\nemerge-webrsync\nemerge --sync\n");
    if (install.portage == false)
        fprintf(script, "getuto\n");
    fprintf(script, "echo \"*/* $(cpuid2cpuflags)\" > /etc/portage/package.use/00cpu-flags\n");
    if (install.world_update)
        fprintf(script, "emerge --ask --verbose --update --deep --newuse @world\n");
    if (install.init == system_d) {
        fprintf(script, "ln -sf ../usr/share/zoneinfo/%s /etc/localtime\n", install.timezone);
    }
    fprintf(script, "locale-gen\nenv-update && source /etc/profile\n");
}
