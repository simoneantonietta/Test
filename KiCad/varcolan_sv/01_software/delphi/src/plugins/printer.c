#include "protocol.h"
#include "codec.h"
#include "event.h"
#include "database.h"
#include "strings.h"
#include "support.h"
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#define EVENT_STRING_BASE	150

static const char *EventString[] = {
    /* SYS events */
	"Allarme sensore $0 Z=$2",
	"Ripristino sensore $0 Z=$2",
	"Guasto perif. $4",
	"Fine Guasto perif. $4",
	"Manom. sensore $0",
	"Fine Manom. sensore $0",
	"Manom. cont.e perif. $4",
	"Fine Manom.e cont.e perif. $4",
	"Attivata zona $2",
	"Disatt.  zona $2",
	"Attivaz. impedita zona $2",
	"In serv. sensore $0",
	"Fuori serv. sensore $0",
	"In serv. attuatore $1",
	"Fuori serv. attuatore $1",
	"MMaster $3: ric.cod errato",
	"Modulo Master $3: tipo $3",
	"Perif. incongr. ind.$4 tipo $3",
	"Perif. manomessa $4",
	"Ripristino perif. $4",
	"Sosp. att.ta' linea $3",
	"Chiave falsa perif. $4",
	"Sent. $4 attivata",
	"Sent. $4 disattivata",
	"Sent. $4 disatt. timeout",
	"NoTx m.master $3",
	"Errore Rx m.master $3",
	"Errore ric. mess. host",
	"Segn.e evento n.$6",
	"Perif. presenti da $4 > $5$5$5$5$5$5$5$5",
	"SZ $8> $a $a $a $a $a $a $a $a",
	"SS $6> $a $a $a $a $a $a $a $a",
	"SA $6> $a $a $a $a $a $a $a $a",
	"Guasto sensore $0",
	"Variazione ora $9",
	"Cod. contr. $x-$x-$x",
	"Stato QRA I=$3 QRA II=$3. Att.ronda $3",
	"Accettato allarme sensore $0",
	"No punz. stazione $4",
	"On telecomando $7",
	"Param. tarat. perif.$4 Sens.$3 Dur.$3",
	"Stato Prova = $3",
	"Test attivo gruppo $8",
	"Risp.SCS",
	"Fine guasto sensore $0",
	"Valore $6 per segreto $3",
	"Perif. previste da $4 > $5$5$5$5$5$5$5$5",
	"Sensore in allarme $0 Z=$2",
	"Variata fascia oraria $8:$3 ore $9",
	"Invio diretto dati t$3 a host computer",
	"",
	"Fine invio dati",
	"Cnf.ronda $3- $3,$3,$3,$3,$3,$3,$3,$3,$3,$3,$3,$3,$3,$3,$3,$3",
	"Ore ronda $3- $9,$9,$9,$9,$9,$9",
	"Lista festivi $3- $3$3$3$3$3$3$3$3$3$3$3$3$3$3$3$3",
	"Lista fasce giornaliere $8- $9 $9  $9 $9",
	"ON Attuatore $1",
	"Transito id. $6 lettore $4",
	"Entrato id. $6 lettore $4",
	"Uscito  id. $6 lettore $4",
	"Codice valido $8",
	"Chiave valida $8",
	"Operatore $8",
	"Staz. $6 punzonata",
	"Spegnimento n. $6",
	"Reset fumo n. $6",
	"Livello $8 abilitato",
	"Variato segreto",
	"Connessione modem tipo $3",
	"Numero telef. primario - ",
	"Numero telef. second.o - ",
	"Esaminato Badge $4:$6",
	"Evento sensore $0 $e $E",
//	"Stato MU $6> $8 $8 $8 $8 $8 $8 $8 $8",
	"MU $6> $a $a $a $a $a $a $a $a",
	"Lista ZS $6> $8 $8 $8 $8 $8 $8 $8 $8",
	"Descr.Zona $2",
	"Descr.Dispositivo $6",
	"Fra $3 giorni variato a $3",
	"Valori analogici $6: $8 $8 $8 $8 $8 $8 $8 $8",
	"Letto badge $3-xxxxxxxxxx"};

static const char *EventExString[] = {
	"GSM $3: $s",
	"TMD $3: $s",
	"$s"};

#define NUM_MSGS_EX2 62
static const char *EventEx2String[][NUM_MSGS_EX2] = {
    {
    /* Tebe */
        "Tebe ACK m:$3 t:$3 id:$i",
        "Lettura dati tessera $i",
        "Lettura dati profilo $D",
        "Lettura dati terminale $d",
        "Lettura dati area $D: $i",
        "Lettura dati fascia oraria $d",
        "Lettura dati festivo $d",
	"Id $i trans. Term $d Area $D",
	"Id $i timbr. Term $d Fe $3",
	"Id $i trans. Term $d Area $Dcodice $i",
	"Id $i timbr. Term $d Fe $3 codice $i",
	"Id $i F.Area Term $d",
	"Id $i N.Abil Term $d",
	"Id $i No Tr. F.O. su Term $d",
	"Id $i No Tr. Fest. su Term $d",
	"Id $i Pf $D  N.Abil Term. $d",
	"Id $i abilita grado resp. da Term $d",
	"Ident. $i Segnalata Coercizione",
	"Id Assente No transito da Term. $d",
	"",
	"",
	"Forzatura porta da Term. $d",
	"Apert. prolung. porta da Term. $d",
	"Fine forz./ap. prolungata portada Term. $d",
	"Allarme Ingresso Aux su Term. $d",
	"Fine allarme Ingresso Aux    su Term. $d",
	"Bassa tensione su Term. $d",
	"Fine bassa tens. su Term. $d",
	"",
	"",
	"",
	"Ident. $i variato PIN",
	"Ident. $i disabilitato",
	"Ident. $i abilitato",
	"Ident. $i inserito da Term. $d",
	"Ident. $i inserito su Centrale",
	"Ident. $i forzato in Area $D",
	"Ident. $i associato a Profilo $D",
	"Ident. $i associato a F.O. Sett. $d",
	"Ident. $i variata assoc. a Terminali",
	"Ident. $i variata assoc. a Festivita'",
	"Ident. $i variato Stato $S1",
	"Ident. $i variato Cont.",
	"Profilo $D associato a F.O. Sett. $d",
	"Profilo $D variata assoc. a Terminali",
	"Profilo $D variata assoc. a Festivita'",
	"Profilo $D variato Stato $S2",
	"Term. $d config.$F",
	"Term. $d config. Fe1>area $D Fe2>area $D",
	"Term. $d assoc. a F.O. Sett. $d",
	"Term. $d variata associazione a Festivita'",
	"Term. $d variato DIN/Cont",
	"Term. $d Tempi: TAP=$D TAZ=$D TPBK=$d",
	"Term. $d variato Stato $S3",
	"Variata F.O. Settimanale $d",
	"Variata Festivita' $d",
	"",
	"",
	"",
	"",
	"Reset Centrale $i",
	"Richiesta messg.cod. $3 da Term. $d",
    },
    {
    /* Delphi */
        "Inizio test attivo gruppo $3",
        "Fine test attivo gruppo $3",
    },
};

static const char *SensorEventType[] = {"allarme", "manomissione", "guasto", "soglia 1", "soglia 2", "soglia 3", "soglia 4", "soglia 5"};
static const char *SensorEventState[] = {"iniziato", "", "accettato", "non accettato", "", "discesa", "salita"};

static char* event_format(const char *msg, unsigned char *ev)
{
  static char buf[256];
  int i, j, k, v;
  
  if(!msg) return NULL;
  
  for(i=j=k=0; msg[i]; i++)
  {
    switch(msg[i])
    {
      case '$':
        i++;
        switch(msg[i])
        {
          case '0':
            v = ev[k++] * 256;
            v += ev[k++];
            sprintf(buf+j, "%s", SensorName[v]);
            j += strlen(buf+j);
            break;
          case '1':
            v = ev[k++] * 256;
            v += ev[k++];
            sprintf(buf+j, "%s", ActuatorName[v]);
            j += strlen(buf+j);
            break;
          case '2':
            v = ev[k++];
            sprintf(buf+j, "%s", ZoneName[v]);
            j += strlen(buf+j);
            break;
          case '3':
            v = ev[k++];
            if(v != 255)
            {
              sprintf(buf+j, "%d", v);
              j += strlen(buf+j);
            }
            break;
          case '4':
            v = ev[k++] * 256;
            v += ev[k++];
            sprintf(buf+j, "%d-%02d", v>>5, v&0x1f);
            j += strlen(buf+j);
            break;
          case '5':
            v = ev[k++];
            if((v&0xf) == 0xf)
              buf[j] = '-';
            else if((v&0xf) == 0xe)
              buf[j] = '*';
            else
              buf[j] = (v&0xf) + '0';
            j++;
            v >>= 4;
            if(v == 0xf)
              buf[j] = '-';
            else if(v == 0xe)
              buf[j] = '*';
            else
              buf[j] = v + '0';
            j++;
            break;
          case '6':
            v = ev[k++] * 256;
            v += ev[k++];
            sprintf(buf+j, "%04d", v);
            j += 4;
            break;
          case '7':
            v = ev[k++] * 256;
            v += ev[k++];
            sprintf(buf+j, "%s", CommandName[v]);
            j += strlen(buf+j);
            break;
          case '8':
            v = ev[k++];
            sprintf(buf+j, "%03d", v);
            j += 3;
            break;
          case '9':
            v = ev[k++];
            if(v != 255)
              sprintf(buf+j, "%02d:%02d", v, ev[k]);
            else
              sprintf(buf+j, "--:--");
            k++;
            j += 5;
            break;
          case 'a':
            v = ev[k++];
            strcpy(buf+j, "----");
            if(v & bitOOS) buf[j] = 'S';
            if(v & bitFailure) buf[j+1] = 'G';
            if(v & bitSabotage) buf[j+2] = 'M';
            if(v & bitAlarm) buf[j+3] = 'A';
            j += 4;
            break;
          case 'd':
            v = ev[k++];
            sprintf(buf+j, "%02d", v);
            j += 2;
            break;
          case 'D':
            v = ev[k++];
            sprintf(buf+j, "%03d", v);
            j += 3;
            break;
          case 'e':
            v = ev[k++];
            sprintf(buf+j, "%s", SensorEventType[v-1]);
            j += strlen(buf+j);
            break;
          case 'E':
            v = ev[k++];
            sprintf(buf+j, "%s", SensorEventState[v-1]);
            j += strlen(buf+j);
            break;
          case 'i':
            v = ev[k++];
            v += ev[k++] * 256;
            sprintf(buf+j, "%05d", v);
            j += 5;
            break;
          case 's':
            v = ev[k++];
            strncpy(buf+j, ev+k, v);
            k += v;
            j += v;
            break;
          case 'S':
            v = ev[k++];
            i++;
            switch(msg[i])
            {
              case '1':
                if(!(v&0xe7))
                {
                  sprintf(buf+j, "-");
                  j++;
                }
                else
                {
                  if(v & 0x02)
                  {
                    sprintf(buf+j, "Coer ");
                    j += 5;
                  }
                  if(!(v & 0x04))
                  {
                    sprintf(buf+j, "PBK ");
                    j += 4;
                  }
                  if(v & 0x01)
                  {
                    sprintf(buf+j, "SV ");
                    j += 3;
                  }
                  if((v & 0xe0) == 0xe0)
                  {
                    sprintf(buf+j, "Canc");
                    j += 4;
                  }
                }
                break;
              case '2':
                sprintf(buf+j, "Coerc:%c  APBK:%c", (v&0x01)?'A':'D', (v&0x02)?'D':'A');
                j += 15;
                break;
              case '3':
                sprintf(buf+j, "APBK:%c BlkTas:%c", (v&0x01)?'D':'A', (v&0x10)?'A':'D');
                j += 15;
                break;
              default:
                break;
            }
            break;
          case 'x':
            v = ev[k++] * 256;
            v += ev[k++];
            sprintf(buf+j, "%04x", v);
            j += 4;
            break;
          default:
            buf[j++] = msg[i];
            break;
        }
        break;
      default:
        buf[j++] = msg[i];
        break;
    }
  }
  
  buf[j++] = '\0';
  return buf;
}

static void printer_loop(ProtDevice *dev)
{
  Event ev;
  int res;
  char buf[80], *p;

  if(!dev) return;
  
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  support_log("PRN Init");
  
  while(1)
  {
    while(!(res = codec_get_event(&ev, dev)));
    if(res < 0)	/* no events queued */
      usleep(100000);
    else
    {
#if 0
      for(res=8; res<ev.Len-2; res++)
      {
        sprintf(buf, "%d ", ev.Event[res]);
        write(dev->fd, buf, strlen(buf));
      }
      write(dev->fd, "\r\n", 2);
#endif

      sprintf(buf, "%04d %04d %02d/%02d/%02d %02d:%02d:%02d ",
        ev.DeviceID[0] * 256 + ev.DeviceID[1],
        ev.Event[0] * 256 + ev.Event[1],
        ev.Event[2], ev.Event[3], ev.Event[4], ev.Event[5], ev.Event[6] & 0x3f, ev.Event[7]);
      res = write(dev->fd, buf, 28);
      if(res < 28) return;	// TCP connection closed
        
      if(ev.Event[8] == Evento_Esteso)
        p = event_format(EventExString[ev.Event[9]], ev.Event + 10);
      else if(ev.Event[8] == Evento_Esteso2)
      {
        if(ev.Event[10] < NUM_MSGS_EX2)
          p = event_format(EventEx2String[ev.Event[9]][ev.Event[10]], ev.Event + 11);
        else
          p = NULL;
      }
      else
      {
        res = ev.Event[8] - EVENT_STRING_BASE;
        if(res >= 0)
          p = event_format(EventString[res], ev.Event + 9);
        else
          p = NULL;
      }
      
      if(p)
      {
        write(dev->fd, p, strlen(p));
        write(dev->fd, "\r\n", 2);
      }
    }
  }
}

void _init()
{
  printf("Printer plugin: " __DATE__ " " __TIME__ "\n");
  prot_plugin_register("P", 0, NULL, NULL, (PthreadFunction)printer_loop);
}

