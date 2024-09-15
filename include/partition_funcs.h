

#ifndef PARTITION_FUNCS_H
#define PARTITION_FUNCS_H
#include "misc.h"
#include <jansson.h>
void initialize_directories();
void format_partition(part const part);
void mount_partition(const part part);
void prepare_partitions(json_t *root, bool const root_part, const int num);
void mount_directories();
#endif // PARTITION_FUNCS_H
