#include <stdio.h>

extern int master_send_threshold;

void _init()
{
  printf("Soglie (plugin): " __DATE__ " " __TIME__ "\n");
  master_send_threshold = 1;
}
