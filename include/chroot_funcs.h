//
// Created by crystal on 14/09/24.
//

#ifndef CHROOT_FUNCS_H
#define CHROOT_FUNCS_H
#include "misc.h"
void change_priority(int const priority);
void extract_chroot(install_type const install);
void mk_script(install_type const install);
void exec_chroot();
#endif //CHROOT_FUNCS_H
