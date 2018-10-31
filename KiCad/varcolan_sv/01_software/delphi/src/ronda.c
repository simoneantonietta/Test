#include "ronda.h"
#include "support.h"
#include "database.h"
#include "timeout.h"
#include "command.h"
#include "user.h"
#include "master.h"
#include <stdio.h>

void (*ronda_punzonatura_p)(int codice, int periferica) = NULL;
int (*ronda_sos_p)(int allarme, int periferica) = NULL;
int (*ronda_chiave_falsa_p)(int periferica) = NULL;
int (*ronda_partenza_p)(int percorso) = NULL;
int (*ronda_partenza_manuale_p)(int percorso) = NULL;
int (*ronda_chiudi_p)(int percorso) = NULL;
void (*ronda_save_p)() = NULL;

Ronda_percorso_t *Ronda_percorso_p = NULL;
Ronda_orario_t *Ronda_orario_p = NULL;
unsigned short *Ronda_stazione_p = NULL;
int *Ronda_zonafiltro_p = NULL;

