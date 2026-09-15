#ifndef NWO_VM_H
#define NWO_VM_H

#include "../nwo_loader.h"

/*
 * run program
 *
 * return:
 *     0   - Normal end
 *     >0  - Return code program
 *     <0  - VM Error
 */
int nwo_execute(NwoProgram* program);

#endif