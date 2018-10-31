#include <stdio.h>
#include <string.h>

#define LARA_MULTIPROFILI	14

typedef struct {
  unsigned char	badge[10];
  union {
    struct {
      unsigned char	supervisor:1;
      unsigned char	coerc:1;
      unsigned char	apbk:1;
      unsigned char	badge:2;
      unsigned char	abil:3;
    } s;
    unsigned char b;
  } stato;
  union {
    struct {
      unsigned char	profilo;
      unsigned char	term[8];
      unsigned char	fosett;
      unsigned char	fest[4];
    } s;
    unsigned char	profilo[LARA_MULTIPROFILI];
  } p;
  unsigned char	area;
  unsigned int	pincnt:24;
} __attribute__ ((packed)) lara_Tessere;

int main(int argc, char *argv[])
{
  FILE *fp;
  int pass, i;
  unsigned char buf[29];

  lara_Tessere tessera;

  if(argc < 3)
  {
    printf("./lara_tessere <lara.gz> <id>\n");
    return 0;
  }

  pass = atoi(argv[2]);
  printf("Pass %d\n", pass);

  fp = fopen(argv[1], "r");
  if(!fp) return 0;

  fread(buf, 4, 1, fp);
  if(memcmp(buf, "TEBE", 4))
    fseek(fp, 8532+(29*pass), SEEK_SET);
  else
    fseek(fp, 8540+(29*pass), SEEK_SET);
  i = fread(buf, 1, 29, fp);
  if(i < 29)
  {
    printf("Tessera non presente\n");
    fclose(fp);
    return 0;
  }

  for(i=0; i<29; i++) printf("%02x", buf[i]);
  printf("\n");

  memcpy(&tessera, buf, sizeof(tessera));
  printf("profilo %d\n", tessera.p.s.profilo);
  printf("supervisore %d\n", tessera.stato.s.supervisor);
  printf("abilitazione ");
  switch(tessera.stato.s.abil)
  {
    case 0:
      printf("Vuoto\n");
      break;
    case 4:
      printf("Disabilitato\n");
      break;
    case 6:
      printf("Abilitato\n");
      break;
    case 7:
      printf("Cancellato\n");
      break;
    default:
      printf("ERRORE (%d)\n", tessera.stato.s.abil);
      break;
  }
  printf("badge+pin %d\n", tessera.stato.s.badge);
  printf("area %d\n", tessera.area);
  printf("pin %d\n", tessera.pincnt);

  printf("badge: ");
  for(i=0; i<10; i++) printf("%02x", tessera.badge[i]);
  printf("\n");

  fclose(fp);
}
