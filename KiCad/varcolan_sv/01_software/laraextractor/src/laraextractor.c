#include <stdio.h>
#include <string.h>

#define LARA_MULTIPROFILI	14

typedef struct
{
	unsigned char badge[10];
	union
	{
		struct
		{
			unsigned char supervisor :1;
			unsigned char coerc :1;
			unsigned char apbk :1;
			unsigned char badge :2;
			unsigned char abil :3;
		} s;
		unsigned char b;
	} stato;
	union
	{
		struct
		{
			unsigned char profilo;
			unsigned char term[8];
			unsigned char fosett;
			unsigned char fest[4];
		} s;
		unsigned char profilo[LARA_MULTIPROFILI];
	} p;
	unsigned char area;
	unsigned int pincnt :24;
}__attribute__ ((packed)) lara_Tessere;

int main(int argc, char *argv[])
{
FILE *fp,*fo;
int pass, i;
unsigned char buf[29];
char tmp1[5],tmp2[50];

lara_Tessere tessera;

fp = fopen("lara", "r");
if(!fp) return 0;
fo = fopen("badge.txt", "w");
if(!fo) return 0;

int j;
for(j=0;j<0xffff;j++)
	{
	pass = j;
	printf("ID %d\n", j);
	rewind(fp);
	fread(buf, 4, 1, fp);
	if(memcmp(buf, "TEBE", 4))
		fseek(fp, 8532 + (29 * pass), SEEK_SET);
	else
		fseek(fp, 8540 + (29 * pass), SEEK_SET);
	i = fread(buf, 1, 29, fp);
	if(i < 29)
		{
		printf("Tessera non presente\n");
		break;
		}

	for(i = 0;i < 29;i++)
		printf("%02x", buf[i]);
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
	tmp2[0]=0;
	for(i = 0;i < 10;i++)
		{
		printf("%02x", tessera.badge[i]);
		sprintf(tmp1,"%02x", tessera.badge[i]);
		strcat(tmp2,tmp1);
		}
	printf("\n");
	if(strcmp(tmp2,"00000000000000000000")!=0)
		{
		fprintf(fo,"%s\t%d\n",tmp2,pass);
		}
	}
fclose(fp);
fclose(fo);
}
