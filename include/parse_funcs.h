

#ifndef PARSE_FUNCS_H
#define PARSE_FUNCS_H
#include "misc.h"
#include <jansson.h>
void parse(char **output, const char *string, const json_t *config);
void output_details(install_type const current);
install_type json_to_conf(json_t *root);
part json_to_part(json_t *root, const int i);
#endif // PARSE_FUNCS_H
