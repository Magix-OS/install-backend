//
// Created by crystal on 14/09/24.
//

#ifndef PARTITION_FUNCS_H
#define PARTITION_FUNCS_H
#include "misc.h"
void initialize_directories();
void format_partition(part const part);
void mount_partition(const part part);
void prepare_partitions(const char *path, bool const root, const int num);
void mount_directories();
#endif // PARTITION_FUNCS_H