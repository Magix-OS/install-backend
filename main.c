#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <errno.h>
#include <jansson.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>

#include "src/customStructs.c"
#include "src/execProg.c"
#include "src/parse.c"
#include "src/partitionFunctions.c"
#include "src/chrootOperations.c"

int main(const int argc, char *argv[]) {
    if (argc > 2) {
        if (strcmp(argv[2], "-p") == 0)
            pretend = 1;
    }
    const char *path = argv[1];
    const install_type current = json_to_conf(path);
    output_details(current);
    initialize_directories1();
    prepare_partitions(path,true);
    extract_chroot(current);
    initialize_directories2();
    mount_directories();
    prepare_partitions(path,false);
    mk_script(current);
    exec_chroot();
    clean_up();
    return 0;
}
