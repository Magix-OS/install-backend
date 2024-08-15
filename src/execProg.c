bool pretend = 0;

void execProg(char *command) {
    printf("Executing %s\n", command);
    if (pretend == 0) {
        if (system(command) != 0) {
            printf("Something went wrong\n");
            if (errno == EACCES)
                printf("Insuffisant permissions, rerun with superuser perms\n");
            exit(EXIT_FAILURE);
        }
    }
}
