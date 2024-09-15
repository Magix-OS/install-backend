

#include "chroot_funcs.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
// Changes the priority of the binhost in gentoobinhost.conf
void change_priority(int const priority) {
  char buffer[1024];
  FILE *gentoobinhost =
      openfile("/mnt/gentoo/etc/portage/binrepos.conf/gentoobinhost.conf", "r");
  FILE *tempFile = openfile(
      "/mnt/gentoo/etc/portage/binrepos.conf/gentoobinhost.conf.tmp", "w");

  char pattern[] = "priority = ";

  while (fgets(buffer, sizeof(buffer), gentoobinhost)) {
    if (strncmp(buffer, pattern, strlen(pattern)) == 0) {
      fprintf(tempFile, "%s%d\n", pattern, priority);
    } else {
      fputs(buffer, tempFile);
    }
  }
  if (pretend == false) {
    fclose(gentoobinhost);
    fclose(tempFile);
    remove("/mnt/gentoo/etc/portage/binrepos.conf/gentoobinhost.conf");
    rename("/mnt/gentoo/etc/portage/binrepos.conf/gentoobinhost.conf.tmp",
           "/mnt/gentoo/etc/portage/binrepos.conf/gentoobinhost.conf");
  }

}
/*Extracts the stage4 into the /mnt/gentoo directory, modifies the files
 * according to user choices. Affected files : /mnt/gentoo/etc/portage/make.conf
 * /mnt/gentoo/etc/portage/binrepos.conf/gentoobinhost.conf
 * /mnt/gentoo/etc/timezone /mnt/gentoo/etc/locale.gen
 * /mnt/gentoo/etc/env.d/02locale /mnt/gentoo/etc/locale.conf
 * /mnt/gentoo/etc/portage/package.use/installkernel /mnt/gentoo/etc/hostname
 * /mnt/gentoo/etc/hosts /mnt/gentoo/etc/conf.d/keymaps
 * /mnt/gentoo/etc/vconsole.conf /mnt/gentoo/etc/doas.conf
 */
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
  sprintf(command, "tar xpvf %s --xattrs-include='*.*' --numeric-owner",
          install.filename);
  exec_prog(command);
  chdir(path);
  FILE *makeconf = openfile("/mnt/gentoo/etc/portage/make.conf", "a");
  fprintf(makeconf, "MAKEOPTS=\"-j%d -l%d\"\n", install.make_opt_j,
          install.make_opt_l);
  fprintf(makeconf, "USE=\"${USE} %s \"\n", install.useflags);
  fprintf(makeconf, "INPUT_DEVICES=\"libinput synaptics\"\n");
  fprintf(makeconf, "ACCEPT_LICENSE=\"*\"\n");
  fprintf(makeconf, "VIDEO_CARDS=\"%s \"\n", install.gpus);
  if (install.binhost) {
    fprintf(
        makeconf,
        R"(# Appending getbinpkg to the list of values within the FEATURES variable
FEATURES="${FEATURES} getbinpkg"
# Require signatures
FEATURES="${FEATURES} binpkg-request-signature"
)");
    change_priority(9999);
  } else
    change_priority(1);
  if (pretend == false)
    fclose(makeconf);

  if (install.systemd == false) {
    FILE *timezone = openfile("/mnt/gentoo/etc/timezone", "w");
    fprintf(timezone, "%s\n", install.timezone);
    if (pretend == false)
      fclose(timezone);
  }

  FILE *localegen = openfile("/mnt/gentoo/etc/locale.gen", "w");
  ;
  fprintf(localegen, "%s\n", install.locales);
  if (pretend == false)
    fclose(localegen);
  FILE *localeconf;
  if (install.systemd == false)
    localeconf = openfile("/mnt/gentoo/etc/env.d/02locale", "w");
  else
    localeconf = openfile("/mnt/gentoo/etc/locale.conf", "w");

  fprintf(localeconf, "LANG=\"%s\"\nLC_COLLATE=\"C.UTF-8\"\n", install.locale);
  if (pretend == false)
    fclose(localeconf);

  FILE *installkernel =
      openfile("/mnt/gentoo/etc/portage/package.use/installkernel", "w");
  fprintf(installkernel, "sys-kernel/installkernel grub dracut\n");
  if (pretend == false)
    fclose(installkernel);
  FILE *hostname = openfile("/mnt/gentoo/etc/hostname", "w");
  fprintf(hostname, "%s\n", install.hostname);
  if (pretend == false)
    fclose(hostname);
  FILE *hosts = openfile("/mnt/gentoo/etc/hosts", "w");
  fprintf(hosts, "127.0.0.1     %s.homenetwork %s localhost\n",
          install.hostname, install.hostname);
  if (pretend == false)
    fclose(hosts);
  FILE *keymaps;
  if (install.systemd == false) {
    keymaps = openfile("/mnt/gentoo/etc/conf.d/keymaps", "w");
    fprintf(keymaps, "keymap=\"%s\"\nwindowkeys=\"YES\"\nfix_euro=\"YES\"\n",
            install.keyboard);
  } else {
    keymaps = openfile("/mnt/gentoo/etc/vconsole.conf", "w");
    fprintf(keymaps, "KEYMAP=\"%s\"\n", install.keyboard);
  }
  if (pretend == false)
    fclose(keymaps);

  if (install.use_doas) {
    FILE *sudoas = openfile("/mnt/gentoo/etc/doas.conf", "w");
    fprintf(sudoas, "permit :wheel\n");
    if (pretend == false)
      fclose(keymaps);
  }
}
// Creates the post-chroot script in /mnt/gentoo/script.sh
void mk_script(install_type const install) {
  FILE *script = openfile("/mnt/gentoo/script.sh", "w+");
  chmod("/mnt/gentoo/script.sh", S_IXOTH);
  fprintf(script, "#!/bin/bash\nset -e\nsource /etc/profile\n");
  if (install.binhost)
    fprintf(script, "getuto\n");
  if (install.systemd) {
    fprintf(script, "ln -sf ../usr/share/zoneinfo/%s /etc/localtime\n",
            install.timezone);
  }
  fprintf(script, "locale-gen\nenv-update && source /etc/profile\n");
  if (install.world_update)
    fprintf(script, "emerge-webrsync\nemerge --sync\nemerge --verbose --update "
                    "--deep --newuse @world\n");
  fprintf(script, "emerge -q");

  if (install.linux_firmware)
    fprintf(script, " sys-kernel/linux-firmware");
  if (install.sof_firmware)
    fprintf(script, " sys-firmware/sof-firmware");
  if (install.intel_microcode)
    fprintf(script, " sys-firmware/intel-microcode");
  if (install.kernel_bin)
    fprintf(script, " sys-kernel/gentoo-kernel-bin");
  else
    fprintf(script, " sys-kernel/gentoo-kernel");

  for (int i = 0; i < FS_NUMBER; i++) {
    if (install.filesystems[i]) {
      fprintf(script, " %s", filesystems[i]);
    }
  }
  if (install.flatpak)
    fprintf(script, " sys-apps/flatpak");
  fprintf(script, " %s ", install.packages);
  if (install.use_doas)
    fprintf(script, " app-admin/doas\nemerge -C sudo\nchown -c root:root "
                    "/etc/doas.conf\nchmod -c 0400 /etc/doas.conf\n");
  else
    fprintf(script, "\n");
  fprintf(script, "genfstab / > /etc/fstab\necho \"*/* $(cpuid2cpuflags)\" > "
                  "/etc/portage/package.use/00cpu-flags\n");
  fprintf(script, "echo -e \"%s\n%s\" | passwd -q\n", install.rootpasswd,
          install.rootpasswd);
  if (install.systemd) {
    fprintf(script, "systemd-machine-id-setup\nsystemctl preset-all "
                    "--preset-mode=enable-only\n");
  }

  if (install.is_uefi)
    fprintf(script, "grub-install --efi-directory=/efi\n");
  else
    fprintf(script, "grub-install %s\n", install.grub_disk);

  fprintf(script, "grub-mkconfig -o /boot/grub/grub.cfg\n");
  fprintf(
      script,
      "useradd -m -G users,wheel,audio,plugdev,video,input -s /bin/bash %s\n",
      install.username);
  fprintf(script, "echo -e \"%s\n%s\" | passwd -q %s\n", install.userpasswd,
          install.userpasswd, install.username);
  if (install.flatpak)
    fprintf(script,
            "su %s -c \"flatpak remote-add --user --if-not-exists flathub "
            "https://flathub.org/repo/flathub.flatpakrepo\"",
            install.username);
  if (pretend == false)
    fclose(script);
}
// Changes directories to /mnt/gentoo, chroots, and executes the script.sh file
void exec_chroot() {
  chdir("/mnt/gentoo");
  chroot("/mnt/gentoo");
  exec_prog("./script.sh");
}