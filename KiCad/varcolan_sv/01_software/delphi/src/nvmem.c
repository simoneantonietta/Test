#include "nvmem.h"
#include "delphi.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

static const char NVMemFileName[] = "/saet/nvmem.nv";

static int *nvmem_data = NULL;
static int nvmem_lastidx = -1;
static int nvmem_modified = 0;

int nvmem_set(int idx, int val)
{
  int *tmp;
  
  if(idx < 0) return -1;
  
  if(idx > nvmem_lastidx)
  {
    tmp = (int*)calloc(idx+1, sizeof(int));
    if(!tmp) return -2;
    
    memcpy(tmp, nvmem_data, (nvmem_lastidx+1) * sizeof(int));
    free(nvmem_data);
    nvmem_data = tmp;
    nvmem_lastidx = idx;
  }
  
  nvmem_data[idx] = val;
  nvmem_modified = 1;
  
  return 0;
}

int nvmem_get(int idx)
{
  if((idx > nvmem_lastidx) || (idx < 0)) return 0;
  return nvmem_data[idx];
}

int nvmem_set_dim(int dim)
{
  return nvmem_set(dim, nvmem_get(dim));
}

int nvmem_save()
{
  FILE *fp;
  int res;
  
  if(!nvmem_modified) return 0;
  
  fp = fopen(ADDROOTDIR(NVMemFileName), "w");
  res = fwrite(nvmem_data, (nvmem_lastidx+1) * sizeof(int), 1, fp);
  fclose(fp);
  sync();
  
  if(res != 1) return -1;

  nvmem_modified = 0;
  
  return 0;
}

int nvmem_load()
{
  FILE *fp;
  struct stat st;
  int dim;

  fp = fopen(ADDROOTDIR(NVMemFileName), "r");
  if(!fp) return 0;

  free(nvmem_data);
  nvmem_lastidx = -1;
  
  fstat(fileno(fp), &st);
  dim = st.st_size / sizeof(int);
  
  nvmem_set_dim(dim-1);
    
  fread(nvmem_data, sizeof(int), dim, fp);
  fclose(fp);

  nvmem_modified = 0;

  return 0;
}

int nvmem_reset()
{
  return unlink(ADDROOTDIR(NVMemFileName));
}

