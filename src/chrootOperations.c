#include <unistd.h>

void extractChroot(installType const install) {
    char command[1024];
    chdir("/mnt/gentoo");
    sprintf(command, "cp %s .", install.filename);
    execProg(command);
    sprintf(command, "tar xpvf %s --xattrs-include='*.*' --numeric-owner", install.filename);
    execProg(command);
    FILE *makeconf;
    if (pretend == 1)
        makeconf = stdout;
    else
        makeconf = fopen("etc/portage/make.conf", "a");
    fprintf(makeconf, "MAKEOPTS=\"-j%d -l%d\"\n", install.makeOptJ, install.makeOptL);
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
        printf("Editing etc/portage/binrepos.conf/gentoobinhost.conf\n");
        if (pretend == 1)
            gentoobinhost = stdout;
        else
            gentoobinhost = fopen("etc/portage/binrepos.conf/gentoobinhost.conf", "a");
        fprintf(gentoobinhost, "priority = 9999");
        fclose(gentoobinhost);
    }
    fclose(makeconf);
}

void mkScript(installType const install) {
    FILE *script;
    if (pretend == 1)
        script = stdout;
    else
        script = fopen("script.sh", "w+");
    chmod("script.sh",S_IXOTH);
    fprintf(script, "#!/bin/bash\nset -e\nsource /etc/profile\nemerge-webrsync\nemerge --sync\n");
    if (install.portage == false)
        fprintf(script, "getuto\n");
    fprintf(script, "echo \"*/* $(cpuid2cpuflags)\" > /etc/portage/package.use/00cpu-flags\nemerge --ask --verbose --update --deep --newuse @world\n");
}
