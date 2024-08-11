#include <errno.h>
#include <jansson.h>
#include <src/customStructs.c>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

bool isUEFI() {
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

installType jsonToConf(const char *path) {
    json_error_t error;
    json_t *root = json_load_file(path, 0, &error);
    installType install;
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
    install.isUefi = isUEFI();
    install.privEscal = json_boolean_value(json_object_get(config, "privEscal"));
    install.kernelBin = json_boolean_value(json_object_get(config, "kernel"));
    install.portage = json_boolean_value(json_object_get(config, "portage"));
    install.init = json_boolean_value(json_object_get(config, "init"));
    install.desktop =
            json_integer_value(json_object_get(config, "desktop"));
    install.makeOptJ = json_integer_value(json_object_get(config, "makeOptJ"));
    install.makeOptL = json_integer_value(json_object_get(config, "makeOptL"));
    install.bedrock = json_boolean_value(json_object_get(config, "bedrock"));
    install.flatpak = json_boolean_value(json_object_get(config, "flatpak"));
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
    int localeLength = 0;
    for (int i = 0; i < json_array_size(locales); i++)
        localeLength = localeLength + 1 +
                       strlen(json_string_value(json_array_get(locales, i)));

    install.locales = (char *) malloc(sizeof(char) * (1 + localeLength));
    sprintf(install.locales, "%s\n",
            json_string_value(json_array_get(locales, 0)));
    for (int i = 1; i < json_array_size(locales); i++) {
        strcat(install.locales, json_string_value(json_array_get(locales, i)));
        strcat(install.locales, "\n\0");
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
    json_decref(root);
    return install;
}

part jsonToPart(const char *path, const int i) {
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
        (char *)malloc(sizeof(char) * (1 + strlen(json_string_value(paths))));
    strcpy(part.partition, json_string_value(paths));
    part.fileSystem = (char *)malloc(sizeof(char) *
                                     (1 + strlen(json_string_value(filesystem))));
    strcpy(part.fileSystem, json_string_value(filesystem));
    part.mountPoint = (char *)malloc(sizeof(char) *
                                     (1 + strlen(json_string_value(mountpoint))));
    strcpy(part.mountPoint, json_string_value(mountpoint));
    part.wipe = wipe;
    json_decref(root);
    return part;
}