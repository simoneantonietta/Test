#ifndef _SAET_NVMEM_H
#define _SAET_NVMEM_H

int nvmem_set(int idx, int val);
int nvmem_get(int idx);
int nvmem_set_dim(int dim);
int nvmem_save();
int nvmem_load();
int nvmem_reset();

#endif
