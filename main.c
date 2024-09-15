#include "chroot_funcs.h"
#include "misc.h"
#include "parse_funcs.h"
#include "partition_funcs.h"
#include <string.h>
// Hello

int main(const int argc, char *argv[]) {
  if (argc > 2) {
    if (strcmp(argv[2], "-p") == 0)
      pretend = true;
  }
  if (argc < 1) {
    perror("Not enough arguments");
    return 1;
  }
  json_error_t error;
  json_t *root = json_load_file(argv[1], 0, &error);
  const install_type current = json_to_conf(root);
  output_details(current);
  initialize_directories();
  prepare_partitions(root, true, current.partitions_number);
  extract_chroot(current);
  exec_prog("swapoff -a");
  prepare_partitions(root, false, current.partitions_number);
  json_decref(root);
  mount_directories();
  mk_script(current);
  free_install(current);
  exec_chroot();
  return 0;
}
