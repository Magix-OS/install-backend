bool pretend = 0;

void exec_prog(char *command) {
    printf("Executing %s\n", command);
    if (pretend == 0) {
        if (system(command) != 0) {
            printf("Something went wrong\n");
            if (errno == EACCES)
                printf("Insufficient permissions, rerun with superuser perms\n");
            exit(EXIT_FAILURE);
        }
    }
}

FILE *openfile(const char *filename, const char *mode) {
    printf("\nOpening: %s\n", filename);
    if (pretend == 1)
        return stdout;
    FILE *openfile = fopen(filename, mode);
    if (openfile == NULL) {
        printf("\n Can't open %s\n", filename);
        exit(EXIT_FAILURE); // or abort()
    }
    return openfile;
}

void free_install(const install_type current) {
  free(current.filename);
  free(current.gpus);
  free(current.grub_disk);
  free(current.hostname);
  free(current.keyboard);
  free(current.locale);
  free(current.locales);
  free(current.rootpasswd);
  free(current.timezone);
  free(current.useflags);
  free(current.username);
  free(current.userpasswd);
}