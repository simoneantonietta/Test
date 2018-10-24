#include <stdio.h>

extern int cmd_comportamento_SYS;

void _init()
{
  printf("DelphiSYS plugin: " __DATE__ " " __TIME__ "\n");
  cmd_comportamento_SYS = 1;
}

