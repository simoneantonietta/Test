#include <stdio.h>

static char *test_radio[] = {
  /*str_d_0=*/ "I Man.",
  /*str_d_1=*/ "F Man.",
};


void RADIO(int len, char **str[])
{
  int i;
  
  if(len > (sizeof(test_radio)/sizeof(char*))) len = sizeof(test_radio)/sizeof(char*);
  
  for(i=0; i<len; i++)
    *str[i] = test_radio[i];
}

void _init()
{
  printf("Language: TEST (" __DATE__ " " __TIME__ ")\n");
}


