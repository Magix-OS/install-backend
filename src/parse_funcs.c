#include "parse_funcs.h"
#include <stdlib.h>
#include <string.h>

// Parses string item from JSON object, allocates enough memory and puts it into
// char **output

void parse(char **output, const char *string, const json_t *config) {
  *output = (char *)malloc(
      sizeof(char) *
      (1 + strlen(json_string_value(json_object_get(config, string)))));
  strcpy(*output, json_string_value(json_object_get(config, string)));
}
// Outputs the current install information in a human-readable way
void output_details(install_type const current) {
  printf("Useflags: %s\nTimezone: %s\nFilename: %s\nLocales: %s\nPrimary "
         "Locale: %s\nKeyboard Layout: %s\nUsername: %s\nHostname: "
         "%s\nUserpassword: %s\nRootpassword: %s\nMakeopts: %d %d\nCards: "
         "%s\nNumber of partitions: %d  ",
         current.useflags, current.timezone, current.filename, current.locales,
         current.locale, current.keyboard, current.username, current.hostname,
         current.userpasswd, current.rootpasswd, current.make_opt_j,
         current.make_opt_l, current.gpus, current.partitions_number);
  //if (current.bedrock)
    //printf("\nBedrock: Yes");
  //else
    //printf("\nBedrock: No");
  if (current.flatpak)
    printf("\nFlatpak: Yes");
  else
    printf("\nFlatpak: No");
  if (current.kernel_bin)
    printf("\nBinary Kernel: Yes");
  else
    printf("\nBinary Kernel: No");
  printf("\nStratas:");
  // for (int i = 0; i < STRATAS_NUMBER; i++) {
  //   if (current.stratas[i]) {
  //     printf(" %s", stratas[i]);
  //   }
  // }
  printf("\nFilesystems:");
  for (int i = 0; i < FS_NUMBER; i++) {
    if (current.filesystems[i]) {
      printf(" %s", filesystems[i]);
    }
  }
  if (current.use_doas)
    printf("\nPriv tool: doas");
  else
    printf("\nPriv tool: sudo");
  if (current.binhost)
    printf("\nPortage packages: binary");
  else
    printf("\nPortage packages: source");
  if (current.systemd)
    printf("\nInit: Systemd");
  else
    printf("\nInit: OpenRC");
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
  printf("\nGrub Disk : %s", current.grub_disk);
}
// Parses the JSON object in json_t *root and returns an install_type variable
install_type json_to_conf(json_t *root) {
  install_type install;
  if (root == NULL)
    exit(EXIT_FAILURE);
  if (!json_is_object(root)) {
    fprintf(stderr, "error: root is not a valid file\n");
    json_decref(root);
    exit(EXIT_FAILURE);
  }
  const json_t *layout = json_object_get(root, "layout");
  if (!json_is_array(layout)) {
    fprintf(stderr, "error: is not a array\n");
    json_decref(root);
    exit(EXIT_FAILURE);
  }
  const json_t *config = json_object_get(root, "config");
  if (!json_is_object(config)) {
    fprintf(stderr, "error: is not a array\n");
    json_decref(root);
    exit(EXIT_FAILURE);
  }
  install.is_uefi = is_uefi();
  install.use_doas = json_boolean_value(json_object_get(config, "use_doas"));
  install.kernel_bin =
      json_boolean_value(json_object_get(config, "binary_kernel"));
  install.binhost = json_boolean_value(json_object_get(config, "binhost"));
  install.systemd = json_boolean_value(json_object_get(config, "systemd"));
  install.world_update =
      json_boolean_value(json_object_get(config, "world_update"));
  install.make_opt_j =
      (int) json_integer_value(json_object_get(config, "make_opt_j"));
  install.make_opt_l =
      (int) json_integer_value(json_object_get(config, "make_opt_l"));
  //install.bedrock = json_boolean_value(json_object_get(config, "bedrock"));
  install.flatpak = json_boolean_value(json_object_get(config, "flatpak"));
  install.intel_microcode =
      json_boolean_value(json_object_get(config, "intel_microcode"));
  install.sof_firmware =
      json_boolean_value(json_object_get(config, "sof_microcode"));
  install.linux_firmware =
      json_boolean_value(json_object_get(config, "linux_firmware"));
  parse(&install.timezone, "timezone", config);
  parse(&install.useflags, "useflags", config);
  parse(&install.gpus, "gpus", config);
  parse(&install.locale, "locale", config);
  parse(&install.keyboard, "keyboard", config);
  parse(&install.username, "username", config);
  parse(&install.hostname, "hostname", config);
  parse(&install.filename, "filename", config);
  parse(&install.userpasswd, "passwd", config);
  parse(&install.rootpasswd, "rootpasswd", config);
  parse(&install.packages, "packages", config);
  parse(&install.grub_disk, "grub_disk", config);
  install.partitions_number = (int) json_array_size(layout);
  const json_t *locales = json_object_get(config, "locales");
  if (!json_is_array(locales)) {
    fprintf(stderr, "error: is not a array\n");
    json_decref(root);
    exit(EXIT_FAILURE);
  }
  int locale_length = 0;
  for (int i = 0; i < json_array_size(locales); i++)
    locale_length = locale_length + 1 + (int) strlen(json_string_value(json_array_get(locales, i)));

  install.locales = (char *)malloc(sizeof(char) * (1 + locale_length));
  sprintf(install.locales, "%s\n",
          json_string_value(json_array_get(locales, 0)));
  for (int i = 1; i < json_array_size(locales); i++) {
    strcat(install.locales, json_string_value(json_array_get(locales, i)));
    if (i != json_array_size(locales) - 1)
      strcat(install.locales, "\n");
  }
  const json_t *strata = json_object_get(config, "stratas");
  if (!json_is_array(strata)) {
    fprintf(stderr, "error: is not a array\n");
    json_decref(root);
    exit(EXIT_FAILURE);
  }
  //for (int i = 0; i < STRATAS_NUMBER; i++)
    //install.stratas[i] = json_boolean_value(json_array_get(strata, i));

  const json_t *fs = json_object_get(config, "filesystems");
  if (!json_is_array(fs)) {
    fprintf(stderr, "error: is not a array\n");
    json_decref(root);
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < json_array_size(fs); i++)
    install.filesystems[i] = json_boolean_value(json_array_get(fs, i));
  return install;
}
// Parses the JSON object in json_t *root and returns a part variable that
// contains the options for the partition of index i
part json_to_part(json_t *root, const int i) {
  part part;
  if (root == NULL)
    exit(EXIT_FAILURE);
  if (!json_is_object(root)) {
    fprintf(stderr, "error: root is not a valid file\n");
    json_decref(root);
    exit(EXIT_FAILURE);
  }

  const json_t *layout = json_object_get(root, "layout");
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
  const bool wipe = json_boolean_value(json_object_get(args, "wipe"));
  parse(&part.partition, "path", partition);
  parse(&part.file_system, "filesystem", args);
  parse(&part.mount_point, "mountpoint", args);
  part.wipe = wipe;
  return part;
}
