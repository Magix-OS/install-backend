#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <src/parse.c>
#include <src/execProg.c>
#include <src/partitionFunctions.c>

int main(int argc, char *argv[]) {
    if (strcmp(argv[2], "-p") == 0)
        pretend = 1;
    const char *path = argv[1];
    const installType current = jsonToConf(path);
    initializeDirectories();
    preparePartitions(path);
    mountDirectories();
    outputDetails(current);
    return 0;
}
