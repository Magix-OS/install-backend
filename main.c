#include "chroot_funcs.h"
#include "misc.h"
#include "parse_funcs.h"
#include "partition_funcs.h"
#include <string.h>
int main(const int argc, char *argv[]) {
  if (argc > 2) {
    if (strcmp(argv[2], "-p") == 0)
      pretend = true;
  }
  if (argc < 1) {
    perror("Not enough arguments");
    return 1;
  }
  const char *path = argv[1];
  const install_type current = json_to_conf(path);
  output_details(current);
  initialize_directories();
  prepare_partitions(path, true, current.partitions_number);
  extract_chroot(current);
  exec_prog("swapoff -a");
  prepare_partitions(path, false, current.partitions_number);
  mount_directories();
  mk_script(current);
  free_install(current);
  exec_chroot();
  return 0;
}
