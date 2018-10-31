#include "console.h"

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_attuatori.c */
const char *str1_0_0="ATN0404ATTUATORE\r";
const char *str1_0_1="ATN0505ESCLUSO\r";
const char *str1_0_2="ATN0407ESCLUDERE?\r";
const char *str1_0_3="ESCLUDI ATT.";
const char *str1_0_4="";//ATN0404ATTUATORE\r";
const char *str1_0_5="ATN0505INCLUSO\r";
const char *str1_0_6="ATN0407INCLUDERE?\r";
const char *str1_0_7="INCLUDI ATT.";
const char *str1_0_8="";//ATN0504ATTUATORE\r";
const char *str1_0_9="ATN0805ON\r";
const char *str1_0_10="ON ATTUATORE";
const char *str1_0_11="";//ATN0504ATTUATORE\r";
const char *str1_0_12="ATN0805OFF\r";
const char *str1_0_13="OFF ATTUATORE";
const char *str1_0_14="ATN0204%s\r";
const char *str1_0_15="ATN%02d05%s\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_badge.c */
const char *str1_1_0="ATN0504LETTORE\r";
const char *str1_1_1="ATN0405NON VALIDO\r";
const char *str1_1_2="ATN0604BADGE\r";
const char *str1_1_3="ATN0405ABILITATO\r";
const char *str1_1_4="ATN0305DISABILITATO\r";
const char *str1_1_5="ATN0604BADGE\r";
const char *str1_1_6="ATN0405NON VALIDO\r";
const char *str1_1_7="ATN0307NUM. LETTORE\r";
const char *str1_1_8="ATN0404NUM. BADGE\r";
const char *str1_1_9="ABILITA BADGE";
const char *str1_1_10="DISABILITA BADGE";
const char *str1_1_11="ATN0708___\r";
const char *str1_1_12="ATG0708\r";
const char *str1_1_13="ATN0705____\r";
const char *str1_1_14="ATG0705\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_impostazioni.c */
const char *str1_2_0="ATN0504FASCIA\r";
const char *str1_2_1="ATN0305MODIFICATA\r";
const char *str1_2_2="ATN0407ORA FINE\r";
const char *str1_2_3="ATN0103FASCIA %03d\r";
const char *str1_2_4="ATN0405ORA INIZIO\r";
const char *str1_2_5="FASCE ORARIE";
const char *str1_2_6="FASCIA %03d";
const char *str1_2_7="Feriale  ";
const char *str1_2_8="Semifest.";
const char *str1_2_9="Festivo  ";
const char *str1_2_10="ATN0304TIPO GIORNO\r";
const char *str1_2_11="ATN0305MODIFICATO\r";
const char *str1_2_12="FESTIVI";
const char *str1_2_13="ATN0704DATA\r";
const char *str1_2_14="ATN0405MODIFICATA\r";
const char *str1_2_15="ATN0204INSERIRE DATA\r";
const char *str1_2_16="ATN0704ORA\r";
const char *str1_2_17="ATN0405MODIFICATA\r";
const char *str1_2_18="ATN0204INSERIRE ORA\r";
const char *str1_2_19="ATN0706DATA\r";
const char *str1_2_20="ATN0707ORA \r";
const char *str1_2_23="DATA E ORA";
const char *str1_2_24="ATN0504VARIARE?\r";
const char *str1_2_25="ATN0704ZONA\r";
const char *str1_2_26="ATN0405NON VALIDA\r";
const char *str1_2_27="";//Zona insieme (old) %d:";
const char *str1_2_28="";//  zona %d:";
const char *str1_2_29="";//Zona insieme (new) %d:";
const char *str1_2_30="";//  zona %d:";
const char *str1_2_31="";//Zona totale:";
const char *str1_2_32="";//  zona %d:";
const char *str1_2_33="ATN0405MODIFICATA\r";
const char *str1_2_34="ZONE/SENSORI";
const char *str1_2_35="ATN0303NUM. SENSORE?\r";
const char *str1_2_36="ATN0704ZONA\r";
const char *str1_2_37="ATN0405NON VALIDA\r";
const char *str1_2_38="ATN0405MODIFICATA\r";
const char *str1_2_39="ATN0503NUM. ZS?\r";
const char *str1_2_40="ATN0504RITARDO\r";
const char *str1_2_41="ATN0405NON VALIDO\r";
const char *str1_2_42="ATN0405IMPOSTATO\r";
const char *str1_2_43="ATN0107Rit:     s\r";
const char *str1_2_44="ZONE RITARDATE";
const char *str1_2_45="ATN0503NUM. ZS?\r";
const char *str1_2_46="ATN0504TIMEOUT\r";
const char *str1_2_47="ATN0405NON VALIDO\r";
const char *str1_2_48="ATN0504TIMEOUT\r";
const char *str1_2_49="ATN0405MODIFICATO\r";
const char *str1_2_50="OUT LOGIN";
const char *str1_2_51="ATN0304TEMPO (sec)?\r";
const char *str1_2_52="ATN0607ERRORE\r";
const char *str1_2_53="CODICI";
const char *str1_2_54="INDIRIZZO IP";
const char *str1_2_55="ATN0304Impostazioni\r";
const char *str1_2_56="ATN0505salvate.\r";
const char *str1_2_57="ATN0104%s\r";
const char *str1_2_58="ATN0105%s\r";
const char *str1_2_59="ATN0107ZS: ___\r";
const char *str1_2_60="ATG0507\r";
const char *str1_2_61="ATN0107ZI: ___\r";
const char *str1_2_62="ATG0507\r";
const char *str1_2_63="ATG0706\r";
const char *str1_2_64="ATN0707____\r";
const char *str1_2_65="ATG0707\r";
const char *str1_2_66="ATN0103IP:\r";
const char *str1_2_67="ATN0104___.   .   .\r";
const char *str1_2_68="ATG0104\r";
const char *str1_2_69="ATN0104%03d\r";
const char *str1_2_70="ATN0404.___.   .    \r";
const char *str1_2_71="ATG0504\r";
const char *str1_2_72="ATN0504%03d\r";
const char *str1_2_73="ATN0804.___.    \r";
const char *str1_2_74="ATG0904\r";
const char *str1_2_75="ATN0904%03d\r";
const char *str1_2_76="ATN1204.___ \r";
const char *str1_2_77="ATG1304\r";
const char *str1_2_78="ATN1304%03d \r";
const char *str1_2_79="ATN0105Netmask:\r";
const char *str1_2_80="ATN0106___.   .   .\r";
const char *str1_2_81="ATG0106\r";

const char *str1_2_82="ATN0106%03d\r";
const char *str1_2_83="ATN0406.___.   .    \r";
const char *str1_2_84="ATG0506\r";
const char *str1_2_85="ATN0506%03d\r";
const char *str1_2_86="ATN0806.___.    \r";
const char *str1_2_87="ATG0906\r";
const char *str1_2_88="ATN0906%03d\r";
const char *str1_2_89="ATN1206.___ \r";
const char *str1_2_90="ATG1306\r";
const char *str1_2_91="ATN1306%03d \r";
const char *str1_2_92="ATN0107Gateway:\r";
const char *str1_2_93="ATN0108___.   .   .\r";
const char *str1_2_94="ATG0108\r";
const char *str1_2_95="ATN0108%03d\r";
const char *str1_2_96="ATN0408.___.   .    \r";
const char *str1_2_97="ATG0508\r";
const char *str1_2_98="ATN0508%03d\r";
const char *str1_2_99="ATN0808.___.    \r";
const char *str1_2_100="ATG0908\r";
const char *str1_2_101="ATN0908%03d\r";
const char *str1_2_102="ATN1208.___ \r";
const char *str1_2_103="ATG1308\r";
const char *str1_2_104="ATN1308%03d \r";

const char *str1_2_105="ATN0103IP:\r";
const char *str1_2_106="ATN0104%s\r";
const char *str1_2_107="ATN0105Netmask:\r";
const char *str1_2_108="ATN0106%s\r";
const char *str1_2_109="ATN0107Gateway:\r";
const char *str1_2_110="ATN0108%s\r";
const char *str1_2_111="ATN0306DHCP attivo\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_inoltra.c */
const char *str1_3_0="ATN0504SEGRETO\r";
const char *str1_3_1="ATN0405NON VALIDO\r";
const char *str1_3_2="ATN0604ERRORE\r";
const char *str1_3_3="ATN0305IMPOSTAZIONE\r";
const char *str1_3_4="";//ATN0504SEGRETO\r";
const char *str1_3_5="ATN0505INVIATO\r";
const char *str1_3_6="ATN0304TIPO SEGRETO\r";
const char *str1_3_7="";//ATN0405NON VALIDO\r";
const char *str1_3_8="ATN0207SEGRETO NUMERO\r";
const char *str1_3_9="INVIA SEGRETO";
const char *str1_3_10="ATN0304TIPO SEGRETO\r";
const char *str1_3_11="ATN0604EVENTO\r";
const char *str1_3_12="";//ATN0405NON VALIDO\r";
const char *str1_3_13="";//ATN0504EVENTO\r";
const char *str1_3_14="";//ATN0505INVIATO\r";
const char *str1_3_15="ATN0204EVENTO NUMERO\r";
const char *str1_3_16="";//ATN0405NON VALIDO\r";
const char *str1_3_17="ATN0407PARAMETRO\r";
const char *str1_3_18="INVIA EVENTO";
const char *str1_3_19="ATN0204EVENTO NUMERO\r";
const char *str1_3_20="ATN0608_____\r";
const char *str1_3_21="ATG0608\r";
const char *str1_3_22="ATN0805_\r";
const char *str1_3_23="ATG0805\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_lara_impostazioni.c */
const char *str1_4_0="ATN0105Domenica\r";
const char *str1_4_1="ATN0105Lunedì\r";
const char *str1_4_2="ATN0105Martedì\r";
const char *str1_4_3="ATN0105Mercoledì\r";
const char *str1_4_4="ATN0105Giovedì\r";
const char *str1_4_5="ATN0105Venerdì\r";
const char *str1_4_6="ATN0105Sabato\r";
const char *str1_4_7="ATN0104Fascia sett. N%02d\r";
const char *str1_4_8="Lunedì";
const char *str1_4_9="Martedì";
const char *str1_4_10="Mercoledì";
const char *str1_4_11="Giovedì";
const char *str1_4_12="Venerdì";
const char *str1_4_13="Sabato";
const char *str1_4_14="Domenica";
const char *str1_4_15="ATN0406(Nessuna)\r";
const char *str1_4_16="ATN0607VUOTO\r";
const char *str1_4_17="ATN0206SINGOLO VARIAB.\r";
const char *str1_4_18="ATN0206PERIODO VARIAB.\r";
const char *str1_4_19="ATN0307da %02d/%02d/%02d\r";
const char *str1_4_20="ATN0408a %02d/%02d/%02d\r";
const char *str1_4_21="ATN0206SINGOLO FISSO\r";
const char *str1_4_22="ATN0206PERIODO FISSO\r";
const char *str1_4_23="ATN0407da %02d/%02d\r";
const char *str1_4_24="ATN0508a %02d/%02d\r";
const char *str1_4_25="ATN0104FEST. ASSOCIATE\r";
const char *str1_4_33="ATN0103Fascia sett. N%02d\r";
const char *str1_4_41="FASCE SETTIMAN.";
const char *str1_4_42="ATN0405i terminali?\r";
const char *str1_4_43="ATN0504a tutti\r";
const char *str1_4_44="ATN0405i profili? \r";
const char *str1_4_45="ATN0403Aggiungere\r";
const char *str1_4_46="ATN0504a tutte\r";
const char *str1_4_47="ATN0405le tessere?\r";
const char *str1_4_48="ATN0107ENTER\r";
const char *str1_4_49="ATO Si  \r";
const char *str1_4_50="ATODEL\r";
const char *str1_4_51="ATO No\r";
const char *str1_4_52="Cancella";
const char *str1_4_53="Singolo variab.";
const char *str1_4_54="Periodo variab.";
const char *str1_4_55="Singolo fisso";
const char *str1_4_56="Periodo fisso";
const char *str1_4_57="FESTIVITA'";
const char *str1_4_58="ATN0204%s\r";
const char *str1_4_59="ATN0105%s\r";
const char *str1_4_60="ATN0507%02d/%02d/%02d\r";
const char *str1_4_61="ATN0607%02d/%02d\r";
const char *str1_4_62="ATN0507 -:- \r";
const char *str1_4_63="ATN1107 -:- \r";
const char *str1_4_64="ATN0508 -:- \r";
const char *str1_4_65="ATN1108 -:- \r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_lara_pass.c */
const char *str1_5_0="BADGE/CNT";
const char *str1_5_1="PROFILO";
const char *str1_5_2="AREA";
const char *str1_5_3="STATO PASS";
const char *str1_5_4="BADGE/PIN";
const char *str1_5_5="";//PROFILO";
const char *str1_5_6="";//AREA";
const char *str1_5_7="";//STATO PASS";
const char *str1_5_8="";//BADGE/CNT";
const char *str1_5_9="";//PROFILO";
const char *str1_5_10="";//AREA";
const char *str1_5_11="";//STATO PASS";
const char *str1_5_12="SUPERVISOR";
const char *str1_5_13="";//BADGE/PIN";
const char *str1_5_14="";//PROFILO";
const char *str1_5_15="";//AREA";
const char *str1_5_16="";//STATO PASS";
const char *str1_5_17="";//SUPERVISOR";
const char *str1_5_18="TERMINALI";
const char *str1_5_19="FASCE SETTIMAN.";
const char *str1_5_20="FESTIVITA'";
const char *str1_5_21="OPZIONI";
const char *str1_5_22="";//TERMINALI";
const char *str1_5_23="";//FASCE SETTIMAN.";
const char *str1_5_24="";//FESTIVITA'";
const char *str1_5_25="";//OPZIONI";
const char *str1_5_26="ATS04\r";
const char *str1_5_27="ATN0105BADGE PRESENTE\r";
const char *str1_5_28="ATN0105BADGE ASSENTE\r";
const char *str1_5_29="ATN0107VALORE CONTATORE\r";
const char *str1_5_30="";//ATN0105BADGE PRESENTE\r";
const char *str1_5_31="";//ATN0105BADGE ASSENTE\r";
const char *str1_5_32="ATN0107PIN PRESENTE\r";
const char *str1_5_33="ATN0107PIN ASSENTE\r";
const char *str1_5_34="ATN0104TERM. ASSOCIATI\r";
const char *str1_5_35="ATN0106ANTI-COERCIZIONE\r";
const char *str1_5_36="ATN0407ATTIVATO\r";
const char *str1_5_37="ATN0307DISATTIVATO\r";
const char *str1_5_38="ATN0206ANTI-PASSBACK\r";
const char *str1_5_39="";//ATN0307DISATTIVATO\r";
const char *str1_5_40="";//ATN0407ATTIVATO\r";
const char *str1_5_41="ATN0504OPZIONI\r";
const char *str1_5_42="COERCIZIONE";
const char *str1_5_43="PASS-BACK";
const char *str1_5_44="ATN0405STATO ABIL\r";
const char *str1_5_45="ATN0407ABILITATO\r";
const char *str1_5_46="ATN0307DISABILITATO\r";
const char *str1_5_47="ATN0305SCONOSCIUTO\r";
const char *str1_5_48="PASS";
const char *str1_5_49="ATN0604ID NUM.\r";
const char *str1_5_50="ATN0606ERRATO\r";
const char *str1_5_51="ATN0406ASSEGNATO\r";
const char *str1_5_52="ATN0307CONFERMA PIN\r";
const char *str1_5_53="ATN0606ERRORE\r";
const char *str1_5_54="ATN0405NUOVO PIN\r";
const char *str1_5_55="ATN0208(max 5 cifre)\r";
const char *str1_5_56="ATN0305VECCHIO PIN\r";
const char *str1_5_57="ATN0305NUOVO VALORE\r";
const char *str1_5_58="";//ATN0406ASSEGNATO\r";
const char *str1_5_59="ATN0405CONTATORE\r";
const char *str1_5_60="ATN0307NUOVO VALORE\r";
const char *str1_5_61="ATN0405TERMINALE\r";
const char *str1_5_62="";//ATN0406ASSEGNATO\r";
const char *str1_5_63="ATN0604ASSEGNA\r";
const char *str1_5_64="";//ATN0405TERMINALE\r";
const char *str1_5_65="ATN0506INIBITO\r";
const char *str1_5_66="ATN0504INIBISCI\r";
const char *str1_5_67="ASSEGNA";
const char *str1_5_68="INIBISCI";
const char *str1_5_69="ATN0404TERMINALI\r";
const char *str1_5_70="ATN0605FASCIA\r";
const char *str1_5_71="ATN0506ASSOCIATA\r";
const char *str1_5_72="ATN0504ASSOCIA\r";
const char *str1_5_73="";//ATN0605FASCIA\r";
const char *str1_5_74="ATN0506ANNULLATA\r";
const char *str1_5_75="ATN0504ANNULLA\r";
const char *str1_5_76="ATN0406(Nessuna)\r";
const char *str1_5_77="ASSOCIA";
const char *str1_5_78="ANNULLA";
const char *str1_5_79="VISUALIZZA";
const char *str1_5_80="ATN0304FASCE SETTIM.\r";
const char *str1_5_81="ATN0405FESTIVITA'\r";
const char *str1_5_82="";//ATN0406ASSOCIATA\r";
const char *str1_5_83="";//ATN0504ASSOCIA\r";
const char *str1_5_84="";//ATN0405FESTIVITA'\r";
const char *str1_5_85="";//ATN0406ANNULLATA\r";
const char *str1_5_86="";//ATN0504ANNULLA\r";
const char *str1_5_87="ASSOCIA";
const char *str1_5_88="ANNULLA";
const char *str1_5_89="VISUALIZZA";
const char *str1_5_90="ATN0404FESTIVITA'\r";
const char *str1_5_91="ATN0506ATTIVATO\r";
const char *str1_5_92="ATN0306DISATTIVATO\r";
const char *str1_5_93="ATN0105ANTI-COERCIZIONE\r";
const char *str1_5_94="";//ATN0506ATTIVATO\r";
const char *str1_5_95="ATN0308DISATTIVARE?\r";
const char *str1_5_96="ATN0306DISATTIVATO\r";
const char *str1_5_97="ATN0508ATTIVARE?\r";
const char *str1_5_98="";//ATN0306DISATTIVATO\r";
const char *str1_5_99="";//ATN0506ATTIVATO\r";
const char *str1_5_100="ATN0205ANTI-PASSBACK\r";
const char *str1_5_101="";//ATN0506ATTIVATO\r";
const char *str1_5_102="";//ATN0308DISATTIVARE?\r";
const char *str1_5_103="";//ATN0306DISATTIVATO\r";
const char *str1_5_104="";//ATN0508ATTIVARE?\r";
const char *str1_5_105="";//ATN0306DISATTIVATO\r";
const char *str1_5_106="";//ATN0506ATTIVATO\r";
const char *str1_5_107="ATN0405SUPERVISOR\r";
const char *str1_5_108="";//ATN0506ATTIVATO\r";
const char *str1_5_109="";//ATN0308DISATTIVARE?\r";
const char *str1_5_110="";//ATN0306DISATTIVATO\r";
const char *str1_5_111="";//ATN0508ATTIVARE?\r";
const char *str1_5_112="";//COERCIZIONE";
const char *str1_5_113="";//PASS-BACK";
const char *str1_5_114="";//SUPERVISOR";
const char *str1_5_115="ATN0504OPZIONI\r";
const char *str1_5_116="ATN0205NUOVO PROFILO\r";
const char *str1_5_117="ATN0406ASSEGNATO\r";
const char *str1_5_118="ATN0205AREA PRESENZA\r";
const char *str1_5_119="ATN0506VARIATA\r";
const char *str1_5_120="ATN0405ABILITATO\r";
const char *str1_5_121="ATN0305DISABILITATO\r";
const char *str1_5_122="ATN0507CONFERMA?\r";
const char *str1_5_123="ATN0405CANCELLATO\r";
const char *str1_5_124="";//ATN0507CONFERMA?\r";
const char *str1_5_125="DISABILITA";
const char *str1_5_126="CANCELLA";
const char *str1_5_127="ABILITA";
const char *str1_5_128="CANCELLA";
const char *str1_5_129="ATN0404STATO ABIL\r";
const char *str1_5_130="";//ATN0405ABILITATO\r";
const char *str1_5_131="";//ATN0305DISABILITATO\r";
const char *str1_5_132="";//ATN0306DISATTIVATO\r";
const char *str1_5_133="";//ATN0506ATTIVATO\r";
const char *str1_5_134="ATN0405SUPERVISOR\r";
const char *str1_5_135="";//ATN0506ATTIVATO\r";
const char *str1_5_136="";//ATN0308DISATTIVARE?\r";
const char *str1_5_137="";//ATN0306DISATTIVATO\r";
const char *str1_5_138="";//ATN0508ATTIVARE?\r";
const char *str1_5_139="ATN0305SCONOSCIUTO\r";
const char *str1_5_140="PASS";
const char *str1_5_141="ATN0604ID NUM.\r";
const char *str1_5_142="PRIMARIO";
const char *str1_5_143="ASSEGNA SECOND.";
const char *str1_5_144="CANCELLA SECOND.";
const char *str1_5_145="ATN0504Profilo\r";
const char *str1_5_146="ATN0505completo\r";
const char *str1_5_147="ATN0505aggiunto\r";
const char *str1_5_148="ATN0405eliminato\r";
const char *str1_5_149="ATN0608%06d\r";
const char *str1_5_150="ATN0305%s\r";
const char *str1_5_151="ATN0106%s\r";
const char *str1_5_152="ATN0506%s\r";
const char *str1_5_153="ATN0107%s\r";
const char *str1_5_154="ATN0503ID %05d\r";
const char *str1_5_155="ATG0606\r";
const char *str1_5_156="ATG0608\r";
const char *str1_5_157="ATN0606%05d\r";
const char *str1_5_158="ATN0304%s\r";
const char *str1_5_159="ATN0105%s\r";
const char *str1_5_160="ATN0504%s\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_lara_profilo.c */
const char *str1_6_0="TERMINALI";
const char *str1_6_1="FASCIA SETTIM.";
const char *str1_6_2="FESTIVITA'";
const char *str1_6_3="OPZIONI";
const char *str1_6_4="";//TERMINALI";
const char *str1_6_5="";//FASCIA SETTIM.";
const char *str1_6_6="";//FESTIVITA'";
const char *str1_6_7="";//OPZIONI";
const char *str1_6_8="ATN0104TERM. ASSOCIATI\r";
const char *str1_6_9="ATN0105FASCIA ASSOCIATA\r";
const char *str1_6_10="ATN0507Nessuna\r";
const char *str1_6_11="ATN0105ANTI-COERCIZIONE\r";
const char *str1_6_12="ATN0506ATTIVATO\r";
const char *str1_6_13="ATN0306DISATTIVATO\r";
const char *str1_6_14="ATN0205ANTI-PASSBACK\r";
const char *str1_6_15="";//ATN0306DISATTIVATO\r";
const char *str1_6_16="";//ATN0506ATTIVATO\r";
const char *str1_6_17="COERCIZIONE";
const char *str1_6_18="PASS BACK";
const char *str1_6_19="ATN0504OPZIONI\r";
const char *str1_6_20="PROFILI";
const char *str1_6_21="ATN0405TERMINALE\r";
const char *str1_6_22="ATN0406ASSEGNATO\r";
const char *str1_6_23="ATN0604ASSEGNA\r";
const char *str1_6_24="";//ATN0405TERMINALE\r";
const char *str1_6_25="ATN0506INIBITO\r";
const char *str1_6_26="ATN0504INIBISCI\r";
const char *str1_6_27="ASSEGNA";
const char *str1_6_28="INIBISCI";
const char *str1_6_29="ATN0404TERMINALI\r";
const char *str1_6_30="ATN0605FASCIA\r";
const char *str1_6_31="ATN0506ASSOCIATA\r";
const char *str1_6_32="ATN0504ASSOCIA\r";
const char *str1_6_33="ATN0605FASCIA\r";
const char *str1_6_34="ATN0506ANNULLATA\r";
const char *str1_6_35="ATN0504ANNULLA\r";
const char *str1_6_36="ATN0406(Nessuna)\r";
const char *str1_6_37="ASSOCIA";
const char *str1_6_38="ANNULLA";
const char *str1_6_39="VISUALIZZA";
const char *str1_6_40="ATN0304FASCE SETTIM.\r";
const char *str1_6_41="ATS04\r";
const char *str1_6_42="ATN0405FESTIVITA'\r";
const char *str1_6_43="";//ATN0406ASSOCIATA\r";
const char *str1_6_44="";//ATN0504ASSOCIA\r";
const char *str1_6_45="";//ATN0405FESTIVITA'\r";
const char *str1_6_46="";//ATN0406ANNULLATA\r";
const char *str1_6_47="";//ATN0504ANNULLA\r";
const char *str1_6_48="";//ASSOCIA";
const char *str1_6_49="";//ANNULLA";
const char *str1_6_50="";//VISUALIZZA";
const char *str1_6_51="ATN0404FESTIVITA'\r";
const char *str1_6_52="";//ATN0506ATTIVATO\r";
const char *str1_6_53="";//ATN0306DISATTIVATO\r";
const char *str1_6_54="ATN0105ANTI-COERCIZIONE\r";
const char *str1_6_55="";//ATN0506ATTIVATO\r";
const char *str1_6_56="ATN0308DISATTIVARE?\r";
const char *str1_6_57="";//ATN0306DISATTIVATO\r";
const char *str1_6_58="ATN0508ATTIVARE?\r";
const char *str1_6_59="";//ATN0306DISATTIVATO\r";
const char *str1_6_60="";//ATN0506ATTIVATO\r";
const char *str1_6_61="ATN0205ANTI-PASSBACK\r";
const char *str1_6_62="";//ATN0506ATTIVATO\r";
const char *str1_6_63="";//ATN0308DISATTIVARE?\r";
const char *str1_6_64="";//ATN0306DISATTIVATO\r";
const char *str1_6_65="";//ATN0508ATTIVARE?\r";
const char *str1_6_66="";//COERCIZIONE";
const char *str1_6_67="";//PASS-BACK";
const char *str1_6_68="";//ATN0504OPZIONI\r";
const char *str1_6_69="ATN0303%s\r";
const char *str1_6_70="ATN0104%s\r";
const char *str1_6_71="";//PROFILI";
const char *str1_6_72="ATN0107%s\r";
const char *str1_6_73="ATN0108%s\r";
const char *str1_6_76="ATN0106%s\r";
const char *str1_6_77="TIPO\r";
const char *str1_6_78="ATN0605TIPO:\r";
const char *str1_6_79="ATN0506NORMALE\r";
const char *str1_6_80="ATN0506SCORTA\r";
const char *str1_6_81="ATN0206VISITATORE (O)\r";
const char *str1_6_82="ATN0206VISITATORE (A)\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_lara_term.c */
const char *str1_7_0="CONTATORE";
const char *str1_7_1="FASCIA SETTIM.";
const char *str1_7_2="FESTIVITA'";
const char *str1_7_3="DIN";
const char *str1_7_4="";//FASCIA SETTIM.";
const char *str1_7_5="";//FESTIVITA'";
const char *str1_7_6="";//CONTATORE";
const char *str1_7_7="";//FASCIA SETTIM.";
const char *str1_7_8="";//FESTIVITA'";
const char *str1_7_9="AREA ENTRATA";
const char *str1_7_10="AREA USCITA";
const char *str1_7_11="TEMPI";
const char *str1_7_12="OPZIONI";
const char *str1_7_13="";//DIN";
const char *str1_7_14="";//FASCIA SETTIM.";
const char *str1_7_15="";//FESTIVITA'";
const char *str1_7_16="";//AREA ENTRATA";
const char *str1_7_17="";//AREA USCITA";
const char *str1_7_18="";//TEMPI";
const char *str1_7_19="";//OPZIONI";
const char *str1_7_20="ATN0406CONTATORE\r";
const char *str1_7_21="ATN0407DECREMENTO\r";
const char *str1_7_22="ATN0207NO DECREMENTO\r";
const char *str1_7_23="ATN0507PRESENTE\r";
const char *str1_7_24="ATN0507ASSENTE\r";
const char *str1_7_25="ATN0105FASCIA ASSOCIATA\r";
const char *str1_7_26="ATN0507Nessuna\r";
const char *str1_7_27="TERMINALI";
const char *str1_7_29="ATN0606ERRATO\r";
const char *str1_7_30="ATN0406ASSEGNATO\r";
const char *str1_7_31="ATN0405DECREMENTO\r";
const char *str1_7_33="ATN0606ERRATO\r";
const char *str1_7_34="ATN0406ASSEGNATO\r";
const char *str1_7_35="ATN0307CONFERMA DIN\r";
const char *str1_7_36="ATN0405NUOVO DIN\r";
const char *str1_7_37="ATN0208(max 5 cifre)\r";
const char *str1_7_38="ATN0605FASCIA\r";
const char *str1_7_39="ATN0506ASSOCIATA\r";
const char *str1_7_40="ATN0504ASSOCIA\r";
const char *str1_7_41="ATN0605FASCIA\r";
const char *str1_7_42="ATN0506ANNULLATA\r";
const char *str1_7_43="ATN0504ANNULLA\r";
const char *str1_7_44="ATN0406(Nessuna)\r";
const char *str1_7_45="ASSOCIA";
const char *str1_7_46="ANNULLA";
const char *str1_7_47="VISUALIZZA";
const char *str1_7_48="ATN0304FASCE SETTIM.\r";
const char *str1_7_49="ATN0405FESTIVITA'\r";
const char *str1_7_50="ATN0406ASSOCIATA\r";
const char *str1_7_51="ATN0504ASSOCIA\r";
const char *str1_7_52="ATN0405FESTIVITA'\r";
const char *str1_7_53="ATN0406ANNULLATA\r";
const char *str1_7_54="ATN0504ANNULLA\r";
const char *str1_7_55="ASSOCIA";
const char *str1_7_56="ANNULLA";
const char *str1_7_57="VISUALIZZA";
const char *str1_7_58="ATN0404FESTIVITA'\r";
const char *str1_7_59="ATN0305AREA ENTRATA\r";
const char *str1_7_60="ATN0506VARIATA\r";
const char *str1_7_61="ATN0305AREA USCITA\r";
const char *str1_7_62="ATN0506VARIATA\r";
const char *str1_7_63="ATN0406VARIA AREA\r";
const char *str1_7_64="ATN0104ENTRATA\r";
const char *str1_7_65="ATN0104USCITA\r";
const char *str1_7_66="ATN0607ERRORE\r";
const char *str1_7_67="ATN0506VARIATO\r";
const char *str1_7_68="ATN0604TEMPO\r";
const char *str1_7_69="ATN0205ANTI PASSBACK\r";
const char *str1_7_70="ATN0606%02d min\r";
const char *str1_7_71="ATN0307VARIA TEMPO\r";
const char *str1_7_72="ATN0607ERRORE\r";
const char *str1_7_73="ATN0506VARIATO\r";
const char *str1_7_74="ATN0604TEMPO\r";
const char *str1_7_75="ATN0205APERT. PROLUNG.\r";
const char *str1_7_76="ATN0606%03d sec\r";
const char *str1_7_77="ATN0307VARIA TEMPO\r";
const char *str1_7_81="ATN0607ERRORE\r";
const char *str1_7_82="ATN0506VARIATO\r";
const char *str1_7_83="ATN0604TEMPO\r";
const char *str1_7_84="ATN0205ON RELE' PORTA\r";
const char *str1_7_85="ATN0606%03d sec\r";
const char *str1_7_86="ATN0307VARIA TEMPO\r";
const char *str1_7_87="PASS BACK";
const char *str1_7_88="APERT. PROLUNG.";
const char *str1_7_89="ON RELE' PORTA";
const char *str1_7_90="ATN0604TEMPI\r";
const char *str1_7_91="ATN0306DISATTIVATO\r";
const char *str1_7_92="ATN0506ATTIVATO\r";
const char *str1_7_93="ATN0204ANTI PASSBACK\r";
const char *str1_7_94="ATN0505ATTIVATO\r";
const char *str1_7_95="ATN0307DISATTIVARE?\r";
const char *str1_7_96="ATN0305DISATTIVATO\r";
const char *str1_7_97="ATN0407ATTIVARE?\r";
const char *str1_7_98="";// "ATN0606SLAVE\r";
const char *str1_7_99="";// "ATN0307PROGRAMMATO\r";
const char *str1_7_100="";// "ATN0407ANNULLATO\r";
const char *str1_7_101="";// "ATN0604MASTER\r";
const char *str1_7_102="";// "ATN0606SLAVE\r";
const char *str1_7_103="";// "ATN0508CONFERMA?\r";
//const char *str1_7_104="ATN0304MASTER/SLAVE\r";
const char *str1_7_104="ATN0304BLOCCO TIMBR.\r";
const char *str1_7_105="";// "ATN0605MASTER\r";
const char *str1_7_106="";// "ATN0207ANNULLA SLAVE?\r";
const char *str1_7_107="";// "ATN0505NO SLAVE\r";
const char *str1_7_108="";// "ATN0107PROGRAMMA SLAVE?\r";
const char *str1_7_109="ATN0507BLOCCATI\r";
const char *str1_7_110="ATN0407SBLOCCATI\r";
const char *str1_7_111="ATN0304BLOCCO TASTI\r";
const char *str1_7_112="ATN0505BLOCCATI\r";
const char *str1_7_113="ATN0407SBLOCCARE?\r";
const char *str1_7_114="ATN0405SBLOCCATI\r";
const char *str1_7_115="ATN0407BLOCCARE?\r";
const char *str1_7_116="ATN0306SELEZIONATO\r";
const char *str1_7_117="ATN0306SELEZIONATO\r";
const char *str1_7_118="ATN0405BADGE+PIN\r";
const char *str1_7_119="ATN0605BADGE\r";
const char *str1_7_120="ATN0705PIN\r";
const char *str1_7_121="  BADGE+PIN";
const char *str1_7_122="";//  BADGE+PIN";
const char *str1_7_123="  BADGE";
const char *str1_7_124="";//  BADGE";
const char *str1_7_125="  PIN";
const char *str1_7_126="";//  PIN";
const char *str1_7_127="ATN0204FILTRO ENTRATA\r";
const char *str1_7_128="ATN0204FILTRO USCITA\r";
const char *str1_7_129="PASS BACK";
//const char *str1_7_130="MASTER/SLAVE";
const char *str1_7_130="BLOCCO TIMBR.";
const char *str1_7_131="BLOCCO TASTI";
const char *str1_7_132="FILTRO ENTRATA";
const char *str1_7_133="FILTRO USCITA";
const char *str1_7_134="ATN0504OPZIONI\r";
const char *str1_7_135="BIOMETRICO";
const char *str1_7_136="";//  "TERMINALI";
const char *str1_7_137="ATN0203Riconoscimento\r";
const char *str1_7_138="ATN0104biometrico volto\r";
const char *str1_7_139="ATN0605ATTIVO\r";
const char *str1_7_140="ATN0407DISATTIVARE?\r";
const char *str1_7_141="ATN0405DISATTIVO\r";
const char *str1_7_142="ATN0407ATTIVARE?\r";
const char *str1_7_143="ATN0608%06d\r";
const char *str1_7_144="ATN0706DIN\r";
const char *str1_7_145="ATN0107%s\r";
const char *str1_7_146="ATN0108%s\r";
const char *str1_7_147="ATN0303%s\r";
const char *str1_7_148="ATN0104%s\r";
const char *str1_7_149="ATN0606%06d\r";
const char *str1_7_150="ATN0307NUOVO VALORE\r";
const char *str1_7_151="ATG0608\r";
const char *str1_7_152="ATG0606\r";
const char *str1_7_153="ATN0106%s\r";
const char *str1_7_154="ATN0804%s\r";
const char *str1_7_155="ATN0105%s\r";
const char *str1_7_156="ATG0808\r";
const char *str1_7_157="ACC.A.RISERVATA";
const char *str1_7_158="ATN0203Contr. accesso\r";
const char *str1_7_159="ATN0204area riservata\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_liste.c */
const char *str1_8_0="ATN0504ALLARME\r";
const char *str1_8_1="ATN0405ACCETTATO\r";
const char *str1_8_2="ATN0504GUASTO\r";
const char *str1_8_3="";//ATN0405ACCETTATO\r";
const char *str1_8_4="ATN0304MANOMISSIONE\r";
const char *str1_8_5="ATN0405ACCETTATA\r";
const char *str1_8_6="ATN0504SENSORE\r";
const char *str1_8_7="ATN0205FUORI SERVIZIO\r";
const char *str1_8_8="ATN0107ACCETTA         \r";
const char *str1_8_9="ATN0108FUORI SERVIZIO  \r";
const char *str1_8_10="";//ATN0107ACCETTA         \r";
const char *str1_8_11="";//ATN0108FUORI SERVIZIO  \r";
const char *str1_8_12="";//ATN0108FUORI SERVIZIO  \r";
const char *str1_8_14="SENS. in ALLARME";
const char *str1_8_15="SENS. da ACCETT.";
const char *str1_8_16="";//ATN0504SENSORE\r";
const char *str1_8_17="ATN0505INCLUSO\r";
const char *str1_8_18="ATN0404INCLUDERE\r";
const char *str1_8_19="SENSORI ESCLUSI";
const char *str1_8_20="SENSORI GUASTI";
const char *str1_8_21="SENS. MANOMESSI";
const char *str1_8_22="ATN0704ZONA\r";
const char *str1_8_23="ATN0305DISATTIVATA\r";
const char *str1_8_24="ATN0404ZONA %s\r";
const char *str1_8_25="ATN0306DISATTIVARE?\r";
const char *str1_8_26="ZONE ATTIVATE";
const char *str1_8_27="ZONA %s";
const char *str1_8_28="";//ATN0704ZONA\r";
const char *str1_8_29="ATN0505ATTIVATA\r";
const char *str1_8_30="ATN0404ZONA %s\r";
const char *str1_8_31="ATN0406ATTIVARE?\r";
const char *str1_8_32="ZONE DISATTIVATE";
const char *str1_8_33="ZONA %s";
const char *str1_8_34="ATN0404ATTUATORE\r";
const char *str1_8_35="ATN0705OFF\r";
const char *str1_8_36="ATN0304DISATTIVARE\r";
const char *str1_8_38="ATTUATORI ON";
const char *str1_8_39="";//ATN0404ATTUATORE\r";
const char *str1_8_40="ATN0505INCLUSO\r";
const char *str1_8_41="ATN0504INCLUDERE\r";
const char *str1_8_42="ATT. ESCLUSI";
const char *str1_8_43="Feriale  ";
const char *str1_8_44="Semifest.";
const char *str1_8_45="Festivo  ";
const char *str1_8_46="ATN0304TIPO GIORNO\r";
const char *str1_8_47="ATN0305MODIFICATO\r";
const char *str1_8_48="FESTIVI";
const char *str1_8_49="NUMERI CHIAMATI";
const char *str1_8_50="ATN0103%s\r";
const char *str1_8_51="ATN0104%s\r";
const char *str1_8_52="ATN0105%s\r";
const char *str1_8_53="ATN0305%s?\r";
const char *str1_8_54="ATN0107%s\r";
const char *str1_8_55="ATN%02d05%s\r";
const char *str1_8_56="ATN0205%s?\r";
const char *str1_8_57="ATN08%02d%s\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_modem.c */
const char *str1_9_0="ATN0504RUBRICA\r";
const char *str1_9_1="ATN0505COMPLETA\r";
const char *str1_9_2="ATN0204NUMERO SALVATO\r";
const char *str1_9_3="ATN0607NUMERO\r";
const char *str1_9_4="";//ATN0504RUBRICA\r";
const char *str1_9_5="";//ATN0505COMPLETA\r";
const char *str1_9_6="CARICA NUMERO";
const char *str1_9_7="ATN0204NOME TITOLARE\r";
const char *str1_9_8="ATN0604NUMERO\r";
const char *str1_9_9="ATN0405CANCELLATO\r";
const char *str1_9_10="CANCELLA NUMERO";
const char *str1_9_11="";//ATN0604NUMERO\r";
const char *str1_9_12="ATN0505ABILITATO\r";
const char *str1_9_13="Modem in uscita";
const char *str1_9_14="Modem in entrata";
const char *str1_9_15="SMS in uscita";
const char *str1_9_16="SMS in entrata";
const char *str1_9_17="GSM in uscita";
const char *str1_9_18="GSM in entrata";
const char *str1_9_19="ABILITA NUMERO";
const char *str1_9_20="";//ATN0604NUMERO\r";
const char *str1_9_21="ATN0305DISABILITATO\r";
const char *str1_9_28="DISABIL. NUMERO";
const char *str1_9_29="ATN0204Messaggio n.%d\r";
const char *str1_9_30="ATN0305registrato.\r";
const char *str1_9_31="ATN0307Durata: %ds\r";
const char *str1_9_32="ATN0204Premere \r";
const char *str1_9_33="ATOINVIO\r";
const char *str1_9_34="ATN0105per terminare la\r";
const char *str1_9_35="ATN0206registrazione\r";
const char *str1_9_36="ATN0604Altra\r";
const char *str1_9_37="ATN0205registrazione\r";
const char *str1_9_38="ATN0506in corso.\r";
const char *str1_9_39="ATN0303Chiamare la\r";
const char *str1_9_40="ATN0404centrale.\r";
const char *str1_9_41="ATN0206Premere \r";
const char *str1_9_42="";//ATOINVIO\r";
const char *str1_9_43="ATN0107per iniziare la\r";
const char *str1_9_44="ATN0208registrazione\r";
const char *str1_9_45="MESSAGGI AUDIO";
const char *str1_9_46="Messaggio n.%d";
const char *str1_9_47="Nuovo messaggio";
const char *str1_9_48="ATN0108________________\r";
const char *str1_9_49="ATG0108\r";
const char *str1_9_50="ATN0105________________\r";
const char *str1_9_51="ATG0105\r";
const char *str1_9_52="ATN0103%s\r";
const char *str1_9_53="ATN0104%s\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_personalizza.c */
const char *str1_10_0="ATN0304DESCRIZIONE\r";
const char *str1_10_1="ATN0305MODIFICATA\r";
const char *str1_10_2="ATN0108(max 16 caratt.)\r";
const char *str1_10_3="PERS. ZONE";
const char *str1_10_4="ZONA %s";
const char *str1_10_5="PERS. SENSORI";
const char *str1_10_6="PERS. ATTUATORI";
const char *str1_10_7="PERS.TELECOMANDI";
const char *str1_10_9="PERS. TITOLARI";
const char *str1_10_10="PERS. CODICI";
const char *str1_10_11="PERS. PROFILI";
const char *str1_10_12="PERS. AREA";
const char *str1_10_13="PERS. FESTIVITA'";
const char *str1_10_14="PERS. FO SETT.";
const char *str1_10_15="PERS. TERMINALI";
const char *str1_10_16="ATN0104%s\r";
const char *str1_10_17="ATN0106________________\r";
const char *str1_10_18="ATG0106\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_sensori.c */
const char *str1_11_0="ATN0504SENSORE\r";
const char *str1_11_1="ATN0505ESCLUSO\r";
const char *str1_11_2="ATN0407ESCLUDERE?\r";
const char *str1_11_3="ESCLUDI SENS.";
const char *str1_11_4="";//ATN0504SENSORE\r";
const char *str1_11_5="ATN0505INCLUSO\r";
const char *str1_11_6="ATN0407INCLUDERE?\r";
const char *str1_11_7="INCLUDI SENS.";
const char *str1_11_8="ATN0404ACCETTATO\r";
const char *str1_11_9=" All";
const char *str1_11_10=" Gua";
const char *str1_11_11=" Man";
const char *str1_11_12="ACC. ALL. SENS.";
const char *str1_11_13="ATN0204ACCETTAZIONE\r";
const char *str1_11_14="ATN0105TOTALE ESEGUITA\r";
const char *str1_11_15="ACCETTA TOTALE";
const char *str1_11_16="ATN0404EFFETTUARE\r";
const char *str1_11_17="ATN0305ACCETTAZIONE\r";
const char *str1_11_18="ATN0506TOTALE?\r";
const char *str1_11_19="ATN0304%s\r";
const char *str1_11_20="ATN%02d05%s\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_sicurezza.c */
const char *str1_12_0="ATN0504PASSWORD\r";
const char *str1_12_1="ATN0605ERRATA\r";
const char *str1_12_2="";//ATN0504PASSWORD\r";
const char *str1_12_3="ATN0405MODIFICATA\r";
const char *str1_12_4="ATN0304CREATA NUOVA\r";
const char *str1_12_5="ATN0605UTENZA\r";
const char *str1_12_6="ATN0305MAX 6 CIFRE\r";
const char *str1_12_7="ATN0107CONF. PASSWORD\r";
const char *str1_12_8="MODIFICA";
const char *str1_12_9="ATN0502PASSWORD\r";
const char *str1_12_10="ATN0604UTENZA\r";
const char *str1_12_11="ATN0505GENERICA\r";
const char *str1_12_12="CREA UTENZA";
const char *str1_12_13="ATN0104DIGIT. PASSWORD\r";
const char *str1_12_14="ATN0204LIVELLO UTENZA\r";
const char *str1_12_15="ATN0305NON CORRETTO\r";
const char *str1_12_16="ATN0303CREARE PRIMA\r";
const char *str1_12_17="ATN0304UNA UTENZA DI\r";
const char *str1_12_18="ATN0405LIVELLO %d\r";
const char *str1_12_19="ATN0207PROFILO UTENZA\r";
const char *str1_12_20="";//CREA UTENZA";
const char *str1_12_21="ATN0404ID UTENZA\r";
const char *str1_12_22="";//ATN0604UTENZA\r";
const char *str1_12_23="ATN0305INESISTENTE\r";
const char *str1_12_24="";//ATN0604UTENZA\r";
const char *str1_12_25="ATN0505CORRENTE\r";
const char *str1_12_26="";//ATN0604UTENZA\r";
const char *str1_12_27="";//ATN0305INESISTENTE\r";
const char *str1_12_28="ATN0604UTENTE\r";
const char *str1_12_29="";//ATN0505CORRENTE\r";
const char *str1_12_30="ATN0604UTENZA\r";
const char *str1_12_31="ATN0405CANCELLATA\r";
const char *str1_12_32="ATN0404CANCELLARE\r";
const char *str1_12_33="ATN0605UTENZA?\r";
const char *str1_12_34="(Profilo %d)";
const char *str1_12_35="CANCELLA UTENZA";
const char *str1_12_36="ATN0508______\r";
const char *str1_12_37="ATG0508\r";
const char *str1_12_38="ATN0505______\r";
const char *str1_12_39="ATG0505\r";
const char *str1_12_40="ATN0808_\r";
const char *str1_12_41="ATG0808\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_sms.c */
const char *str1_13_0="ATN0304SMS INVIATO\r";
const char *str1_13_1="ATN0104DIGITARE NUMERO\r";
const char *str1_13_2="INVIA SMS";
const char *str1_13_3="ATN0203DIGITARE TESTO\r";
const char *str1_13_4="ATN0405MESSAGGIO\r";
const char *str1_13_5="ATN0406CANCELLATO\r";
const char *str1_13_6="ATN0405CANCELLARE\r";
const char *str1_13_7="ATN0406MESSAGGIO?\r";
const char *str1_13_9="ATN0108CANCELLA\r";
const char *str1_13_10="ATN0104GSM impegnato su\r";
const char *str1_13_11="ATN0105altro terminale\r";
const char *str1_13_12="LEGGI SMS";
const char *str1_13_13="ATN0304Caricamento\r";
const char *str1_13_14="ATN0405messaggi\r";
const char *str1_13_15="CREDITO";
const char *str1_13_16="ATN0404Inoltrata\r";
const char *str1_13_17="ATN0405richiesta\r";
const char *str1_13_18="";
const char *str1_13_19="ATN0307Errore SIM\r";
const char *str1_13_20="ATG0106\r";
const char *str1_13_21="ATG0104\r";
const char *str1_13_22="ATN01%02d%-16.16s\r";
const char *str1_13_23="ATN0103%s\r";
const char *str1_13_24="ATN0104%s\r";
const char *str1_13_25="ATN0105%s:\r";
const char *str1_13_26="ATN0106%s\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/console.c */
const char *str1_14_0="DISATTIVA       ";
const char *str1_14_1="ATTIVA          ";
const char *str1_14_2="ATTIVA con ALL. ";
const char *str1_14_3="ATTIVA con ESCL.";
const char *str1_14_4="SENS. in ALLARME";
const char *str1_14_5="SENS. da ACCETT.";
const char *str1_14_6="SENS. ESCLUSI   ";
const char *str1_14_7="SENS. GUASTI    ";
const char *str1_14_8="SENS. MANOMESSI ";
const char *str1_14_9="ZONE ATTIVATE   ";
const char *str1_14_10="ZONE DISATTIVATE";
const char *str1_14_11="ATT. in stato ON";
const char *str1_14_12="ATT. ESCLUSI    ";
const char *str1_14_13="NUMERI CHIAMATI ";
const char *str1_14_14="FESTIVI         ";
const char *str1_14_15="INVIA           ";
const char *str1_14_16="PERIFERICHE     ";
const char *str1_14_17="ZONE/SENSORI    ";
const char *str1_14_18="INCLUDI         ";
const char *str1_14_19="ESCLUDI         ";
const char *str1_14_20="ACCETTA SENSORE ";
const char *str1_14_21="ACCETTA TOTALE  ";
const char *str1_14_22="INCLUDI         ";
const char *str1_14_23="ESCLUDI         ";
const char *str1_14_24="OFF ATTUATORE   ";
const char *str1_14_25="ON ATTUATORE    ";
const char *str1_14_26="INVIA           ";
const char *str1_14_27="LEGGI           ";
const char *str1_14_28="CREDITO         ";
const char *str1_14_29="FASCE ORARIE    ";
const char *str1_14_30="FESTIVI         ";
const char *str1_14_31="DATA E ORA      ";
const char *str1_14_32="ASS. ZONE/SENS. ";
const char *str1_14_33="ASS. ZS/ZI      ";
const char *str1_14_34="ZONE RITARDATE  ";
const char *str1_14_35="TIMEOUT LOGIN   ";
const char *str1_14_36="CODICI DIRETTI  ";
const char *str1_14_37="ZONE            ";
const char *str1_14_38="SENSORI         ";
const char *str1_14_39="ATTUATORI       ";
const char *str1_14_40="TITOLARI NUMERI ";
const char *str1_14_41="TELECOMANDI     ";
const char *str1_14_42="CODICI          ";
const char *str1_14_43="ABILITA         ";
const char *str1_14_44="DISABILITA      ";
const char *str1_14_45="CARICA NUMERO   ";
const char *str1_14_46="CANCELLA NUMERO ";
const char *str1_14_47="ABILITA NUMERO  ";
const char *str1_14_48="DISABILITA NUM. ";
const char *str1_14_49="MESSAGGI AUDIO  ";
const char *str1_14_50="MODIFICA PASSWD ";
const char *str1_14_51="CREA UTENZA     ";
const char *str1_14_52="CANCELLA UTENZA ";
const char *str1_14_53="SEGRETI         ";
const char *str1_14_54="EVENTI          ";
const char *str1_14_55="REAL TIME       ";
const char *str1_14_56="CONSOLIDATO     ";
const char *str1_14_57="CANCELLA STORICO";
const char *str1_14_58="PROFILI         ";
const char *str1_14_59="AREE            ";
const char *str1_14_60="FESTIVITA'      ";
const char *str1_14_61="FASCE SETTIM.   ";
const char *str1_14_62="TERMINALI       ";
const char *str1_14_63="ZONE            ";
const char *str1_14_64="SENSORI         ";
const char *str1_14_65="ATTUATORI       ";
const char *str1_14_66="TITOLARI NUMERI ";
const char *str1_14_67="TELECOMANDI     ";
const char *str1_14_68="CODICI          ";
const char *str1_14_69="VISUALIZZA      ";
const char *str1_14_70="MODIFICA        ";
const char *str1_14_71="VISUALIZZA      ";
const char *str1_14_72="MODIFICA        ";
const char *str1_14_73="VISUALIZZA      ";
const char *str1_14_74="MODIFICA        ";
const char *str1_14_75="FASCE SETTIMAN. ";
const char *str1_14_76="FESTIVITA'      ";
const char *str1_14_77="DATA E ORA      ";
const char *str1_14_78="ASS. ZONE/SENS. ";
const char *str1_14_79="ASS. ZS/ZI      ";
const char *str1_14_80="TIMEOUT LOGIN   ";
const char *str1_14_81="CODICI DIRETTI  ";
const char *str1_14_82="ZONE";
const char *str1_14_83="LISTE";
const char *str1_14_84="TELECOMANDI";
const char *str1_14_85="SENSORI";
const char *str1_14_86="ATTUATORI";
const char *str1_14_87="IMPOSTAZIONI";
const char *str1_14_88="SMS";
const char *str1_14_89="PERSONALIZZA";
const char *str1_14_90="BADGE";
const char *str1_14_91="CHIAVI";
const char *str1_14_92="MODEM";
const char *str1_14_93="SICUREZZA";
const char *str1_14_94="INOLTRA";
const char *str1_14_95="STORICO";
const char *str1_14_96="LOGOUT";
const char *str1_14_97="ZONE";
const char *str1_14_98="LISTE";
const char *str1_14_99="TELECOMANDI";
const char *str1_14_100="SENSORI";
const char *str1_14_101="ATTUATORI";
const char *str1_14_102="IMPOSTAZIONI";
const char *str1_14_103="SMS";
const char *str1_14_104="PERSONALIZZA";
const char *str1_14_105="MODEM";
const char *str1_14_106="SICUREZZA";
const char *str1_14_107="INOLTRA";
const char *str1_14_108="STORICO";
const char *str1_14_109="PASS";
const char *str1_14_110="PROFILI";
const char *str1_14_111="TERMINALI";
const char *str1_14_112="LOGOUT";
const char *str1_14_113="Gen";
const char *str1_14_114="Feb";
const char *str1_14_115="Mar";
const char *str1_14_116="Apr";
const char *str1_14_117="Mag";
const char *str1_14_118="Giu";
const char *str1_14_119="Lug";
const char *str1_14_120="Ago";
const char *str1_14_121="Set";
const char *str1_14_122="Ott";
const char *str1_14_123="Nov";
const char *str1_14_124="Dic";
const char *str1_14_125="ATN05%02d(Vuoto)\r";
const char *str1_14_126="ATN0101Zona n.%d\r";
const char *str1_14_127="ATN0101Zona n.%d\r";
const char *str1_14_128="ATN0604CODICE\r";
const char *str1_14_129="ATN0405NON VALIDO\r";
const char *str1_14_130="ATN0301INVIO CODICE\r";
const char *str1_14_131="ATN0604CODICE\r";
const char *str1_14_132="ATN0803CODICE\r";
const char *str1_14_133="    %d- Login utente %s";
const char *str1_14_134="ATN0605ERRATO\r";
const char *str1_14_135="ATN0504DIGITARE\r";
const char *str1_14_136="ATN0505PASSWORD\r";
const char *str1_14_137="ATN0601LOGIN\r";
const char *str1_14_138="ATN0304DIGITARE ID\r";
const char *str1_14_139="Console: modificato baudrate";
const char *str1_14_140="ATN0304Sistema TEBE\r";
const char *str1_14_141="ATN0405non attivo\r";
const char *str1_14_142="INDIRIZZO IP    ";
const char *str1_14_143="ATN01%02d%-16s\r";
const char *str1_14_144="ATN0101%s\r";
const char *str1_14_145="ATN01%02d%s\r";
const char *str1_14_146="ATN%02d01%s\r";
const char *str1_14_147="ATN0102----------------\r";
const char *str1_14_148="ATN0604LOGOUT?\r";
const char *str1_14_149="ATN010%d%s\r";
const char *str1_14_150="ATN0104%s\r";
const char *str1_14_151="ATN0105%s\r";
const char *str1_14_152="ATN030%d%c\r";
const char *str1_14_153="ATN050%d}\r";
const char *str1_14_154="ATN0905____\r";
const char *str1_14_155="ATG0905\r";
const char *str1_14_156="ATN0504%02d/%02d/%02d\r";
const char *str1_14_157="ATN0505%02d:%02d:%02d\r";
const char *str1_14_158="ATN0604LOGIN\r";
const char *str1_14_159="ATN0607______\r";
const char *str1_14_160="ATG0607\r";
const char *str1_14_161="ATN0606______\r";
const char *str1_14_162="ATG0606\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_storico.c */
const char *str1_15_50="Allarme         sensore $0";
const char *str1_15_51="Ripristino      sensore $0";
const char *str1_15_52="Guasto          perif. $4";
const char *str1_15_53="Fine guasto     perif. $4";
const char *str1_15_54="Manomissione    sensore $0";
const char *str1_15_55="Fine manomiss.  sensore $0";
const char *str1_15_56="Manomissione    contenitore     perif. $4";
const char *str1_15_57="Fine manomiss.  contenitore     perif. $4";
const char *str1_15_58="Attivata        zona $2";
const char *str1_15_59="Disattivata     zona $2";
const char *str1_15_60="Att. impedita   zona $2";
const char *str1_15_61="In servizio     sensore $0";
const char *str1_15_62="Fuori servizio  sensore $0";
const char *str1_15_63="In servizio     attuatore $1";
const char *str1_15_64="Fuori servizio  attuatore $1";
const char *str1_15_65="Mod. Master $3:$nricevuto$ncodice errato";
const char *str1_15_66="Mod. Master $3:$ntipo $3";
const char *str1_15_67="Perif. incongr. ind. $4$ntipo $3";
const char *str1_15_68="Perif. manomessaind. $4";
const char *str1_15_69="Ripristino perifind. $4";
const char *str1_15_70="Sospesa attivitàlinea $3";
const char *str1_15_71="Chiave falsa    perif. $4";
const char *str1_15_72="Sent. $4$nattivata";
const char *str1_15_73="Sent. $4$ndisattivata";
const char *str1_15_74="Sent. $4$ndisattivata$ntimeout";
const char *str1_15_75="Mod. Master $3:$nErrore Tx";
const char *str1_15_76="Mod. Master $3:$nErrore Rx";
const char *str1_15_77="Errore ric.$nmessaggio host";
const char *str1_15_78="Segnalazione$nevento n.$6";
const char *str1_15_79="Perif. presenti$n$4 >$n$5$5$5$5$5$5$5$5";
const char *str1_15_80="Stato zone   $8$a$a$a$a$a$a$a$a";
const char *str1_15_81="Stato sens. $6$a$a$a$a$a$a$a$a";
const char *str1_15_82="Stato att.  $6$a$a$a$a$a$a$a$a";
const char *str1_15_83="Guasto          sensore $0";
const char *str1_15_84="Variazione ora  $9";
const char *str1_15_85="Codici$ncontrollo$n$x-$x-$x";
//const char *str1_15_87="Stato QRA I=$3 QRA II=$3. Att.ronda $3";
const char *str1_15_88="Accett. allarme sensore $0";
const char *str1_15_89="No punzonatura  stazione $4";
const char *str1_15_90="On telecomando  $7";
const char *str1_15_91="Param. taratura perif. $4$nSens.$3 Dur.$3";
const char *str1_15_92="Stato Prova = $3";
const char *str1_15_93="Test attivo     gruppo $8";
const char *str1_15_94="Risposta SCS";
const char *str1_15_95="Fine guasto     sensore $0";
const char *str1_15_96="Valore $6$nper segreto $3";
const char *str1_15_97="Perif. previste$n$4 >$n$5$5$5$5$5$5$5$5";
const char *str1_15_98="Sens. in allarmesensore $0$nZ=$2";
const char *str1_15_99="Variata fascia$noraria $8:$n$f ore $9";
//const char *str1_15_101="Invio diretto dati t$3 a host computer";
const char *str1_15_102="";
const char *str1_15_103="Fine invio dati";
//const char *str1_15_105="Cnf.ronda $3- $3,$3,$3,$3,$3,$3,$3,$3,$3,$3,$3,$3,$3,$3,$3,$3";
//const char *str1_15_107="Ore ronda $3- $9,$9,$9,$9,$9,$9";
const char *str1_15_108="Lista festivi $3:$3$3$3$3$3$3$3$3$3$3$3$3$3$3$3$3";
const char *str1_15_109="Fasce giorn. $8$n$9 $9$n$9 $9";
const char *str1_15_110="Attivato        attuatore $1";
const char *str1_15_111="Transito id $6lettore $4";
const char *str1_15_112="Entrato id. $6lettore $4";
const char *str1_15_113="Uscito  id. $6lettore $4";
const char *str1_15_114="Codice valido   $8";
const char *str1_15_115="Chiave valida   $8";
const char *str1_15_116="Operatore $8";
const char *str1_15_117="Stazione $6   punzonata";
const char *str1_15_118="Spegnimento     n. $6";
const char *str1_15_119="Reset fumo      n. $6";
const char *str1_15_120="Livello $8     abilitato";
const char *str1_15_121="Variato segreto";
const char *str1_15_122="Conness. modem  tipo $3";
const char *str1_15_123="Numero telef.  primario -";
const char *str1_15_124="Numero telef.  secondario -";
const char *str1_15_125="Esaminato Badge $4:$6";
const char *str1_15_126="Ev. sens. $6$n$e$n$E";
const char *str1_15_127="Stato MU $6$n$a$a$a$a$a$a$a$a";
const char *str1_15_128="Sens. $6 Zone:$8 $8 $8 $8 $8 $8 $8 $8";
//const char *str1_15_130="Descr.Zona $2";
//const char *str1_15_132="Descr.Dispositivo $6";
const char *str1_15_133="Fra $3 giorni$nvariato a $3";
const char *str1_15_134="Val. anal. $6$n$X$X$X$X$X$X$X$X";
const char *str1_15_135="Letto badge     $3-xxxxxxxxxx";
const char *str1_15_136="GSM $3:$n$s";
const char *str1_15_137="TMD $3:$n$s";
const char *str1_15_138="Tebe ACK        m:$3 t:$3 id:$i";
const char *str1_15_139="Lettura dati    tessera $i";
const char *str1_15_140="Lettura dati    profilo $D";
const char *str1_15_141="Lettura dati    terminale $d";
const char *str1_15_142="Lettura dati    area $D: $i";
const char *str1_15_143="Lettura dati    fascia oraria $d";
const char *str1_15_144="Lettura dati    festivo $d";
const char *str1_15_145="Id $i trans. Term $d Area $D";
const char *str1_15_146="Id $i timbr. Term $d Fe $3";
const char *str1_15_147="Id $i trans. Term $d Area $Dcodice $i";
const char *str1_15_148="Id $i timbr. Term $d Fe $3$ncodice $i";
const char *str1_15_149="Id $i F.Area Term $d";
const char *str1_15_150="Id $i N.Abil Term $d";
const char *str1_15_151="Id $i No Tr. F.O. su Term $d";
const char *str1_15_152="Id $i No Tr. Fest. su Term $d";
const char *str1_15_153="Id $i Pf $D  N.Abil Term. $d";
const char *str1_15_154="Id $i abilitagrado resp.$nda Term $d";
const char *str1_15_155="Ident. $i$nSegnalata$nCoercizione";
const char *str1_15_156="Id Assente$nNo transito$nda Term. $d";
const char *str1_15_157="";
const char *str1_15_158="";
const char *str1_15_159="Forzatura porta da Term. $d";
const char *str1_15_160="Apert. prolung. porta da$nTerm. $d";
const char *str1_15_161="Fine forz./ap.  prolungata portada Term. $d";
const char *str1_15_162="Allarme IngressoAux su Term. $d";
const char *str1_15_163="Fine allarme    Ingresso Aux    su Term. $d";
const char *str1_15_164="Bassa tensione  su Term. $d";
const char *str1_15_165="Fine bassa tens.su Term. $d";
const char *str1_15_166="";
const char *str1_15_167="";
const char *str1_15_168="";
const char *str1_15_169="Ident. $i$nvariato PIN";
const char *str1_15_170="Ident. $i$ndisabilitato";
const char *str1_15_171="Ident. $i$nabilitato";
const char *str1_15_172="Ident. $i$ninserito da$nTerm. $d";
const char *str1_15_173="Ident. $i$ninserito su$nCentrale";
const char *str1_15_174="Ident. $i$nforzato in$nArea $D";
const char *str1_15_175="Ident. $i$nassociato a$nProfilo $D";
const char *str1_15_176="Ident. $i$nassociato a$nF.O. Sett. $d";
const char *str1_15_177="Ident. $i$nvariata assoc.  a Terminali";
const char *str1_15_178="Ident. $i$nvariata assoc.  a Festivita'";
const char *str1_15_179="Ident. $i$nvariato Stato   $S1";
const char *str1_15_180="Ident. $i$nvariato Cont.";
const char *str1_15_181="Profilo $D$nassociato a$nF.O. Sett. $d";
const char *str1_15_182="Profilo $D$nvariata assoc.  a Terminali";
const char *str1_15_183="Profilo $D$nvariata assoc.  a Festivita'";
const char *str1_15_184="Profilo $D$nvariato Stato   $S2";
const char *str1_15_185="Term. $d config.$F";
const char *str1_15_186="Term. $d config.Fe1>area $D$nFe2>area $D";
const char *str1_15_187="Term. $d assoc. a F.O. Sett. $d";
const char *str1_15_188="Term. $d variataassociazione a$nFestivita'";
const char *str1_15_189="Term. $d$nvariato DIN/Cont";
const char *str1_15_190="Term. $d$nTempi:  TAP=$D TAZ=$D TPBK=$d";
const char *str1_15_191="Term. $d$nvariato Stato   $S3";
const char *str1_15_192="Variata F.O.$nSettimanale $d";
const char *str1_15_193="Variata$nFestivita' $d";
const char *str1_15_194="";
const char *str1_15_195="";
const char *str1_15_196="";
const char *str1_15_197="";
const char *str1_15_198="Reset Centrale  $i";
const char *str1_15_199="Ident. $i$nTerm. $d$nRichiesta $i";
const char *str1_15_200="Presenze$nArea $D: $i";
const char *str1_15_201="Inizio$ntest attivo$ngruppo $3";
const char *str1_15_202="Fine$ntest attivo$ngruppo $3";
const char *str1_15_203="allarme";
const char *str1_15_204="manomissione";
const char *str1_15_205="guasto";
const char *str1_15_206="iniziato";
const char *str1_15_208="accettato";
const char *str1_15_209="non accettato";
const char *str1_15_210="fine";
const char *str1_15_211="inizio";
const char *str1_15_212="B+S";
const char *str1_15_213="B  ";
const char *str1_15_214="S  ";
const char *str1_15_215="   ";
const char *str1_15_216="Fin=%s Fout=%s";
const char *str1_15_217="Coer ";
const char *str1_15_218="PBK ";
const char *str1_15_219="SV ";
const char *str1_15_220="Canc";
const char *str1_15_221="Coerc:%c  APBK:%c";
const char *str1_15_222="APBK:%c BlkTas:%c";
const char *str1_15_223="ATN0405CANCELLARE\r";
const char *str1_15_224="ATN0506STORICO?\r";
const char *str1_15_225="";//ATN0405CANCELLARE\r";
const char *str1_15_226="ATN0606EVENTO?\r";
const char *str1_15_227="REALTIME";
const char *str1_15_228="";//ATN0405CANCELLARE\r";
const char *str1_15_229="";//ATN0506STORICO?\r";
const char *str1_15_230="";//ATN0405CANCELLARE\r";
const char *str1_15_231="";//ATN0606EVENTO?\r";
const char *str1_15_232="ATN0103Progressivo %04d\r";
const char *str1_15_233="ATN0504(Vuoto)\r";
const char *str1_15_234="CONSOLIDATO";
const char *str1_15_235="ATN0504STORICO\r";
const char *str1_15_236="ATN0405CANCELLATO\r";
const char *str1_15_237="CANCELLA STORICO";
const char *str1_15_238="";//ATN0405CANCELLARE\r";
const char *str1_15_239="";//ATN0506STORICO?\r";
const char *str1_15_240="Evento$nsconosciuto";
const char *str1_15_241="Allineamento$ncentrale";
const char *str1_15_242="ATN0104";
const char *str1_15_243="ATN01%02d%.16s\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_telecomandi.c */
const char *str1_16_0="ATN0304TELECOMANDO\r";
const char *str1_16_1="ATN0505INVIATO\r";
const char *str1_16_2="ATN0506INVIARE?\r";
const char *str1_16_3="TELECOMANDI";
const char *str1_16_4="PERIFERICHE";
const char *str1_16_5="ATN0404Lista\r";
const char *str1_16_6="ATN0305periferiche\r";
const char *str1_16_7="ATN0306in storico\r";
const char *str1_16_8="ZONE/SENSORI";
const char *str1_16_9="ATN0404 Lista\r";
const char *str1_16_10="ATN0205sensore/zona\r";
const char *str1_16_11="ATN0306in storico\r";
const char *str1_16_12="ATN0504%s\r";
const char *str1_16_13="ATN%02d05%s\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_zone.c */
const char *str1_17_0="ATN0704ZONA\r";
const char *str1_17_1="ATN0305DISATTIVATA\r";
const char *str1_17_2="ATN0404ZONA %s\r";
const char *str1_17_3="ATN0306DISATTIVARE?\r";
const char *str1_17_4="DISATTIVA ZONA";
const char *str1_17_5="ZONA %s";
const char *str1_17_6="ATN0704ZONA\r";
const char *str1_17_7="ATN0505ATTIVATA\r";
const char *str1_17_8="ATN0404ZONA %s\r";
const char *str1_17_9="ATN0406ATTIVARE?\r";
const char *str1_17_10="ATTIVA ZONA";
const char *str1_17_11="";//ZONA %s";
const char *str1_17_12="";//ATN0704ZONA\r";
const char *str1_17_13="";//ATN0505ATTIVATA\r";
const char *str1_17_14="";//ATN0404ZONA %s\r";
const char *str1_17_15="";//ATN0406ATTIVARE?\r";
const char *str1_17_16="";//ATTIVA ZONA";
const char *str1_17_17="";//ZONA %s";
const char *str1_17_19="";//ATN0704ZONA\r";
const char *str1_17_20="";//ATN0505ATTIVATA\r";
const char *str1_17_21="";//ATN0404ZONA %s\r";
const char *str1_17_22="";//ATN0406ATTIVARE?\r";
const char *str1_17_23="";//ATTIVA ZONA";
const char *str1_17_24="";//ZONA %s";

const char **console_str1[CONSOLE_STRINGS] = {
  &str1_0_0,
  &str1_0_1,
  &str1_0_2,
  &str1_0_3,
  &str1_0_4,
  &str1_0_5,
  &str1_0_6,
  &str1_0_7,
  &str1_0_8,
  &str1_0_9,
  &str1_0_10,
  &str1_0_11,
  &str1_0_12,
  &str1_0_13,
  &str1_1_0,
  &str1_1_1,
  &str1_1_2,
  &str1_1_3,
  &str1_1_4,
  &str1_1_5,
  &str1_1_6,
  &str1_1_7,
  &str1_1_8,
  &str1_1_9,
  &str1_1_10,
  &str1_2_0,
  &str1_2_1,
  &str1_2_2,
  &str1_2_3,
  &str1_2_4,
  &str1_2_5,
  &str1_2_6,
  &str1_2_7,
  &str1_2_8,
  &str1_2_9,
  &str1_2_10,
  &str1_2_11,
  &str1_2_12,
  &str1_2_13,
  &str1_2_14,
  &str1_2_15,
  &str1_2_16,
  &str1_2_17,
  &str1_2_18,
  &str1_2_19,
  &str1_2_20,
  &str1_2_23,
  &str1_2_24,
  &str1_2_25,
  &str1_2_26,
  &str1_2_27,
  &str1_2_28,
  &str1_2_29,
  &str1_2_30,
  &str1_2_31,
  &str1_2_32,
  &str1_2_33,
  &str1_2_34,
  &str1_2_35,
  &str1_2_36,
  &str1_2_37,
  &str1_2_38,
  &str1_2_39,
  &str1_2_40,
  &str1_2_41,
  &str1_2_42,
  &str1_2_43,
  &str1_2_44,
  &str1_2_45,
  &str1_2_46,
  &str1_2_47,
  &str1_2_48,
  &str1_2_49,
  &str1_2_50,
  &str1_2_51,
  &str1_2_52,
  &str1_2_53,
  &str1_3_0,
  &str1_3_1,
  &str1_3_2,
  &str1_3_3,
  &str1_3_4,
  &str1_3_5,
  &str1_3_6,
  &str1_3_7,
  &str1_3_8,
  &str1_3_9,
  &str1_3_10,
  &str1_3_11,
  &str1_3_12,
  &str1_3_13,
  &str1_3_14,
  &str1_3_15,
  &str1_3_16,
  &str1_3_17,
  &str1_3_18,
  &str1_3_19,
  &str1_4_0,
  &str1_4_1,
  &str1_4_2,
  &str1_4_3,
  &str1_4_4,
  &str1_4_5,
  &str1_4_6,
  &str1_4_7,
  &str1_4_8,
  &str1_4_9,
  &str1_4_10,
  &str1_4_11,
  &str1_4_12,
  &str1_4_13,
  &str1_4_14,
  &str1_4_15,
  &str1_4_16,
  &str1_4_17,
  &str1_4_18,
  &str1_4_19,
  &str1_4_20,
  &str1_4_21,
  &str1_4_22,
  &str1_4_23,
  &str1_4_24,
  &str1_4_25,
  &str1_4_33,
  &str1_4_41,
  &str1_4_42,
  &str1_4_43,
  &str1_4_44,
  &str1_4_45,
  &str1_4_46,
  &str1_4_47,
  &str1_4_48,
  &str1_4_49,
  &str1_4_50,
  &str1_4_51,
  &str1_4_52,
  &str1_4_53,
  &str1_4_54,
  &str1_4_55,
  &str1_4_56,
  &str1_4_57,
  &str1_5_0,
  &str1_5_1,
  &str1_5_2,
  &str1_5_3,
  &str1_5_4,
  &str1_5_5,
  &str1_5_6,
  &str1_5_7,
  &str1_5_8,
  &str1_5_9,
  &str1_5_10,
  &str1_5_11,
  &str1_5_12,
  &str1_5_13,
  &str1_5_14,
  &str1_5_15,
  &str1_5_16,
  &str1_5_17,
  &str1_5_18,
  &str1_5_19,
  &str1_5_20,
  &str1_5_21,
  &str1_5_22,
  &str1_5_23,
  &str1_5_24,
  &str1_5_25,
  &str1_5_26,
  &str1_5_27,
  &str1_5_28,
  &str1_5_29,
  &str1_5_30,
  &str1_5_31,
  &str1_5_32,
  &str1_5_33,
  &str1_5_34,
  &str1_5_35,
  &str1_5_36,
  &str1_5_37,
  &str1_5_38,
  &str1_5_39,
  &str1_5_40,
  &str1_5_41,
  &str1_5_42,
  &str1_5_43,
  &str1_5_44,
  &str1_5_45,
  &str1_5_46,
  &str1_5_47,
  &str1_5_48,
  &str1_5_49,
  &str1_5_50,
  &str1_5_51,
  &str1_5_52,
  &str1_5_53,
  &str1_5_54,
  &str1_5_55,
  &str1_5_56,
  &str1_5_57,
  &str1_5_58,
  &str1_5_59,
  &str1_5_60,
  &str1_5_61,
  &str1_5_62,
  &str1_5_63,
  &str1_5_64,
  &str1_5_65,
  &str1_5_66,
  &str1_5_67,
  &str1_5_68,
  &str1_5_69,
  &str1_5_70,
  &str1_5_71,
  &str1_5_72,
  &str1_5_73,
  &str1_5_74,
  &str1_5_75,
  &str1_5_76,
  &str1_5_77,
  &str1_5_78,
  &str1_5_79,
  &str1_5_80,
  &str1_5_81,
  &str1_5_82,
  &str1_5_83,
  &str1_5_84,
  &str1_5_85,
  &str1_5_86,
  &str1_5_87,
  &str1_5_88,
  &str1_5_89,
  &str1_5_90,
  &str1_5_91,
  &str1_5_92,
  &str1_5_93,
  &str1_5_94,
  &str1_5_95,
  &str1_5_96,
  &str1_5_97,
  &str1_5_98,
  &str1_5_99,
  &str1_5_100,
  &str1_5_101,
  &str1_5_102,
  &str1_5_103,
  &str1_5_104,
  &str1_5_105,
  &str1_5_106,
  &str1_5_107,
  &str1_5_108,
  &str1_5_109,
  &str1_5_110,
  &str1_5_111,
  &str1_5_112,
  &str1_5_113,
  &str1_5_114,
  &str1_5_115,
  &str1_5_116,
  &str1_5_117,
  &str1_5_118,
  &str1_5_119,
  &str1_5_120,
  &str1_5_121,
  &str1_5_122,
  &str1_5_123,
  &str1_5_124,
  &str1_5_125,
  &str1_5_126,
  &str1_5_127,
  &str1_5_128,
  &str1_5_129,
  &str1_5_130,
  &str1_5_131,
  &str1_5_132,
  &str1_5_133,
  &str1_5_134,
  &str1_5_135,
  &str1_5_136,
  &str1_5_137,
  &str1_5_138,
  &str1_5_139,
  &str1_5_140,
  &str1_5_141,
  &str1_6_0,
  &str1_6_1,
  &str1_6_2,
  &str1_6_3,
  &str1_6_4,
  &str1_6_5,
  &str1_6_6,
  &str1_6_7,
  &str1_6_8,
  &str1_6_9,
  &str1_6_10,
  &str1_6_11,
  &str1_6_12,
  &str1_6_13,
  &str1_6_14,
  &str1_6_15,
  &str1_6_16,
  &str1_6_17,
  &str1_6_18,
  &str1_6_19,
  &str1_6_20,
  &str1_6_21,
  &str1_6_22,
  &str1_6_23,
  &str1_6_24,
  &str1_6_25,
  &str1_6_26,
  &str1_6_27,
  &str1_6_28,
  &str1_6_29,
  &str1_6_30,
  &str1_6_31,
  &str1_6_32,
  &str1_6_33,
  &str1_6_34,
  &str1_6_35,
  &str1_6_36,
  &str1_6_37,
  &str1_6_38,
  &str1_6_39,
  &str1_6_40,
  &str1_6_41,
  &str1_6_42,
  &str1_6_43,
  &str1_6_44,
  &str1_6_45,
  &str1_6_46,
  &str1_6_47,
  &str1_6_48,
  &str1_6_49,
  &str1_6_50,
  &str1_6_51,
  &str1_6_52,
  &str1_6_53,
  &str1_6_54,
  &str1_6_55,
  &str1_6_56,
  &str1_6_57,
  &str1_6_58,
  &str1_6_59,
  &str1_6_60,
  &str1_6_61,
  &str1_6_62,
  &str1_6_63,
  &str1_6_64,
  &str1_6_65,
  &str1_6_66,
  &str1_6_67,
  &str1_6_68,
  &str1_6_69,
  &str1_6_70,
  &str1_6_71,
  &str1_7_0,
  &str1_7_1,
  &str1_7_2,
  &str1_7_3,
  &str1_7_4,
  &str1_7_5,
  &str1_7_6,
  &str1_7_7,
  &str1_7_8,
  &str1_7_9,
  &str1_7_10,
  &str1_7_11,
  &str1_7_12,
  &str1_7_13,
  &str1_7_14,
  &str1_7_15,
  &str1_7_16,
  &str1_7_17,
  &str1_7_18,
  &str1_7_19,
  &str1_7_20,
  &str1_7_21,
  &str1_7_22,
  &str1_7_23,
  &str1_7_24,
  &str1_7_25,
  &str1_7_26,
  &str1_7_27,
  &str1_7_29,
  &str1_7_30,
  &str1_7_31,
  &str1_7_33,
  &str1_7_34,
  &str1_7_35,
  &str1_7_36,
  &str1_7_37,
  &str1_7_38,
  &str1_7_39,
  &str1_7_40,
  &str1_7_41,
  &str1_7_42,
  &str1_7_43,
  &str1_7_44,
  &str1_7_45,
  &str1_7_46,
  &str1_7_47,
  &str1_7_48,
  &str1_7_49,
  &str1_7_50,
  &str1_7_51,
  &str1_7_52,
  &str1_7_53,
  &str1_7_54,
  &str1_7_55,
  &str1_7_56,
  &str1_7_57,
  &str1_7_58,
  &str1_7_59,
  &str1_7_60,
  &str1_7_61,
  &str1_7_62,
  &str1_7_63,
  &str1_7_64,
  &str1_7_65,
  &str1_7_66,
  &str1_7_67,
  &str1_7_68,
  &str1_7_69,
  &str1_7_70,
  &str1_7_71,
  &str1_7_72,
  &str1_7_73,
  &str1_7_74,
  &str1_7_75,
  &str1_7_76,
  &str1_7_77,
  &str1_7_81,
  &str1_7_82,
  &str1_7_83,
  &str1_7_84,
  &str1_7_85,
  &str1_7_86,
  &str1_7_87,
  &str1_7_88,
  &str1_7_89,
  &str1_7_90,
  &str1_7_91,
  &str1_7_92,
  &str1_7_93,
  &str1_7_94,
  &str1_7_95,
  &str1_7_96,
  &str1_7_97,
  &str1_7_98,
  &str1_7_99,
  &str1_7_100,
  &str1_7_101,
  &str1_7_102,
  &str1_7_103,
  &str1_7_104,
  &str1_7_105,
  &str1_7_106,
  &str1_7_107,
  &str1_7_108,
  &str1_7_109,
  &str1_7_110,
  &str1_7_111,
  &str1_7_112,
  &str1_7_113,
  &str1_7_114,
  &str1_7_115,
  &str1_7_116,
  &str1_7_117,
  &str1_7_118,
  &str1_7_119,
  &str1_7_120,
  &str1_7_121,
  &str1_7_122,
  &str1_7_123,
  &str1_7_124,
  &str1_7_125,
  &str1_7_126,
  &str1_7_127,
  &str1_7_128,
  &str1_7_129,
  &str1_7_130,
  &str1_7_131,
  &str1_7_132,
  &str1_7_133,
  &str1_7_134,
  &str1_7_136,
  &str1_8_0,
  &str1_8_1,
  &str1_8_2,
  &str1_8_3,
  &str1_8_4,
  &str1_8_5,
  &str1_8_6,
  &str1_8_7,
  &str1_8_8,
  &str1_8_9,
  &str1_8_10,
  &str1_8_11,
  &str1_8_12,
  &str1_8_14,
  &str1_8_15,
  &str1_8_16,
  &str1_8_17,
  &str1_8_18,
  &str1_8_19,
  &str1_8_20,
  &str1_8_21,
  &str1_8_22,
  &str1_8_23,
  &str1_8_24,
  &str1_8_25,
  &str1_8_26,
  &str1_8_27,
  &str1_8_28,
  &str1_8_29,
  &str1_8_30,
  &str1_8_31,
  &str1_8_32,
  &str1_8_33,
  &str1_8_34,
  &str1_8_35,
  &str1_8_36,
  &str1_8_38,
  &str1_8_39,
  &str1_8_40,
  &str1_8_41,
  &str1_8_42,
  &str1_8_43,
  &str1_8_44,
  &str1_8_45,
  &str1_8_46,
  &str1_8_47,
  &str1_8_48,
  &str1_8_49,
  &str1_9_0,
  &str1_9_1,
  &str1_9_2,
  &str1_9_3,
  &str1_9_4,
  &str1_9_5,
  &str1_9_6,
  &str1_9_7,
  &str1_9_8,
  &str1_9_9,
  &str1_9_10,
  &str1_9_11,
  &str1_9_12,
  &str1_9_13,
  &str1_9_14,
  &str1_9_15,
  &str1_9_16,
  &str1_9_17,
  &str1_9_18,
  &str1_9_19,
  &str1_9_20,
  &str1_9_21,
  &str1_9_28,
  &str1_9_29,
  &str1_9_30,
  &str1_9_31,
  &str1_9_32,
  &str1_9_33,
  &str1_9_34,
  &str1_9_35,
  &str1_9_36,
  &str1_9_37,
  &str1_9_38,
  &str1_9_39,
  &str1_9_40,
  &str1_9_41,
  &str1_9_42,
  &str1_9_43,
  &str1_9_44,
  &str1_9_45,
  &str1_9_46,
  &str1_9_47,
  &str1_10_0,
  &str1_10_1,
  &str1_10_2,
  &str1_10_3,
  &str1_10_4,
  &str1_10_5,
  &str1_10_6,
  &str1_10_7,
  &str1_10_9,
  &str1_10_10,
  &str1_10_11,
  &str1_10_12,
  &str1_10_13,
  &str1_10_14,
  &str1_10_15,
  &str1_11_0,
  &str1_11_1,
  &str1_11_2,
  &str1_11_3,
  &str1_11_4,
  &str1_11_5,
  &str1_11_6,
  &str1_11_7,
  &str1_11_8,
  &str1_11_9,
  &str1_11_10,
  &str1_11_11,
  &str1_11_12,
  &str1_11_13,
  &str1_11_14,
  &str1_11_15,
  &str1_11_16,
  &str1_11_17,
  &str1_11_18,
  &str1_12_0,
  &str1_12_1,
  &str1_12_2,
  &str1_12_3,
  &str1_12_4,
  &str1_12_5,
  &str1_12_6,
  &str1_12_7,
  &str1_12_8,
  &str1_12_9,
  &str1_12_10,
  &str1_12_11,
  &str1_12_12,
  &str1_12_13,
  &str1_12_14,
  &str1_12_15,
  &str1_12_16,
  &str1_12_17,
  &str1_12_18,
  &str1_12_19,
  &str1_12_20,
  &str1_12_21,
  &str1_12_22,
  &str1_12_23,
  &str1_12_24,
  &str1_12_25,
  &str1_12_26,
  &str1_12_27,
  &str1_12_28,
  &str1_12_29,
  &str1_12_30,
  &str1_12_31,
  &str1_12_32,
  &str1_12_33,
  &str1_12_34,
  &str1_12_35,
  &str1_13_0,
  &str1_13_1,
  &str1_13_2,
  &str1_13_3,
  &str1_13_4,
  &str1_13_5,
  &str1_13_6,
  &str1_13_7,
  &str1_13_9,
  &str1_13_10,
  &str1_13_11,
  &str1_13_12,
  &str1_13_13,
  &str1_13_14,
  &str1_13_15,
  &str1_13_16,
  &str1_13_17,
  &str1_13_18,
  &str1_13_19,
  &str1_14_0,
  &str1_14_1,
  &str1_14_2,
  &str1_14_3,
  &str1_14_4,
  &str1_14_5,
  &str1_14_6,
  &str1_14_7,
  &str1_14_8,
  &str1_14_9,
  &str1_14_10,
  &str1_14_11,
  &str1_14_12,
  &str1_14_13,
  &str1_14_14,
  &str1_14_15,
  &str1_14_16,
  &str1_14_17,
  &str1_14_18,
  &str1_14_19,
  &str1_14_20,
  &str1_14_21,
  &str1_14_22,
  &str1_14_23,
  &str1_14_24,
  &str1_14_25,
  &str1_14_26,
  &str1_14_27,
  &str1_14_28,
  &str1_14_29,
  &str1_14_30,
  &str1_14_31,
  &str1_14_32,
  &str1_14_33,
  &str1_14_34,
  &str1_14_35,
  &str1_14_36,
  &str1_14_37,
  &str1_14_38,
  &str1_14_39,
  &str1_14_40,
  &str1_14_41,
  &str1_14_42,
  &str1_14_43,
  &str1_14_44,
  &str1_14_45,
  &str1_14_46,
  &str1_14_47,
  &str1_14_48,
  &str1_14_49,
  &str1_14_50,
  &str1_14_51,
  &str1_14_52,
  &str1_14_53,
  &str1_14_54,
  &str1_14_55,
  &str1_14_56,
  &str1_14_57,
  &str1_14_58,
  &str1_14_59,
  &str1_14_60,
  &str1_14_61,
  &str1_14_62,
  &str1_14_63,
  &str1_14_64,
  &str1_14_65,
  &str1_14_66,
  &str1_14_67,
  &str1_14_68,
  &str1_14_69,
  &str1_14_70,
  &str1_14_71,
  &str1_14_72,
  &str1_14_73,
  &str1_14_74,
  &str1_14_75,
  &str1_14_76,
  &str1_14_77,
  &str1_14_78,
  &str1_14_79,
  &str1_14_80,
  &str1_14_81,
  &str1_14_82,
  &str1_14_83,
  &str1_14_84,
  &str1_14_85,
  &str1_14_86,
  &str1_14_87,
  &str1_14_88,
  &str1_14_89,
  &str1_14_90,
  &str1_14_91,
  &str1_14_92,
  &str1_14_93,
  &str1_14_94,
  &str1_14_95,
  &str1_14_96,
  &str1_14_97,
  &str1_14_98,
  &str1_14_99,
  &str1_14_100,
  &str1_14_101,
  &str1_14_102,
  &str1_14_103,
  &str1_14_104,
  &str1_14_105,
  &str1_14_106,
  &str1_14_107,
  &str1_14_108,
  &str1_14_109,
  &str1_14_110,
  &str1_14_111,
  &str1_14_112,
  &str1_14_113,
  &str1_14_114,
  &str1_14_115,
  &str1_14_116,
  &str1_14_117,
  &str1_14_118,
  &str1_14_119,
  &str1_14_120,
  &str1_14_121,
  &str1_14_122,
  &str1_14_123,
  &str1_14_124,
  &str1_14_125,
  &str1_14_126,
  &str1_14_127,
  &str1_14_128,
  &str1_14_129,
  &str1_14_130,
  &str1_14_131,
  &str1_14_132,
  &str1_14_133,
  &str1_14_134,
  &str1_14_135,
  &str1_14_136,
  &str1_14_137,
  &str1_14_138,
  &str1_14_139,
  &str1_14_140,
  &str1_14_141,
  &str1_15_50,
  &str1_15_51,
  &str1_15_52,
  &str1_15_53,
  &str1_15_54,
  &str1_15_55,
  &str1_15_56,
  &str1_15_57,
  &str1_15_58,
  &str1_15_59,
  &str1_15_60,
  &str1_15_61,
  &str1_15_62,
  &str1_15_63,
  &str1_15_64,
  &str1_15_65,
  &str1_15_66,
  &str1_15_67,
  &str1_15_68,
  &str1_15_69,
  &str1_15_70,
  &str1_15_71,
  &str1_15_72,
  &str1_15_73,
  &str1_15_74,
  &str1_15_75,
  &str1_15_76,
  &str1_15_77,
  &str1_15_78,
  &str1_15_79,
  &str1_15_80,
  &str1_15_81,
  &str1_15_82,
  &str1_15_83,
  &str1_15_84,
  &str1_15_85,
  &str1_15_88,
  &str1_15_89,
  &str1_15_90,
  &str1_15_91,
  &str1_15_92,
  &str1_15_93,
  &str1_15_94,
  &str1_15_95,
  &str1_15_96,
  &str1_15_97,
  &str1_15_98,
  &str1_15_99,
  &str1_15_102,
  &str1_15_103,
  &str1_15_108,
  &str1_15_109,
  &str1_15_110,
  &str1_15_111,
  &str1_15_112,
  &str1_15_113,
  &str1_15_114,
  &str1_15_115,
  &str1_15_116,
  &str1_15_117,
  &str1_15_118,
  &str1_15_119,
  &str1_15_120,
  &str1_15_121,
  &str1_15_122,
  &str1_15_123,
  &str1_15_124,
  &str1_15_125,
  &str1_15_126,
  &str1_15_127,
  &str1_15_128,
  &str1_15_133,
  &str1_15_134,
  &str1_15_135,
  &str1_15_136,
  &str1_15_137,
  &str1_15_138,
  &str1_15_139,
  &str1_15_140,
  &str1_15_141,
  &str1_15_142,
  &str1_15_143,
  &str1_15_144,
  &str1_15_145,
  &str1_15_146,
  &str1_15_147,
  &str1_15_148,
  &str1_15_149,
  &str1_15_150,
  &str1_15_151,
  &str1_15_152,
  &str1_15_153,
  &str1_15_154,
  &str1_15_155,
  &str1_15_156,
  &str1_15_157,
  &str1_15_158,
  &str1_15_159,
  &str1_15_160,
  &str1_15_161,
  &str1_15_162,
  &str1_15_163,
  &str1_15_164,
  &str1_15_165,
  &str1_15_166,
  &str1_15_167,
  &str1_15_168,
  &str1_15_169,
  &str1_15_170,
  &str1_15_171,
  &str1_15_172,
  &str1_15_173,
  &str1_15_174,
  &str1_15_175,
  &str1_15_176,
  &str1_15_177,
  &str1_15_178,
  &str1_15_179,
  &str1_15_180,
  &str1_15_181,
  &str1_15_182,
  &str1_15_183,
  &str1_15_184,
  &str1_15_185,
  &str1_15_186,
  &str1_15_187,
  &str1_15_188,
  &str1_15_189,
  &str1_15_190,
  &str1_15_191,
  &str1_15_192,
  &str1_15_193,
  &str1_15_194,
  &str1_15_195,
  &str1_15_196,
  &str1_15_197,
  &str1_15_198,
  &str1_15_199,
  &str1_15_200,
  &str1_15_201,
  &str1_15_202,
  &str1_15_203,
  &str1_15_204,
  &str1_15_205,
  &str1_15_206,
  &str1_15_208,
  &str1_15_209,
  &str1_15_210,
  &str1_15_211,
  &str1_15_212,
  &str1_15_213,
  &str1_15_214,
  &str1_15_215,
  &str1_15_216,
  &str1_15_217,
  &str1_15_218,
  &str1_15_219,
  &str1_15_220,
  &str1_15_221,
  &str1_15_222,
  &str1_15_223,
  &str1_15_224,
  &str1_15_225,
  &str1_15_226,
  &str1_15_227,
  &str1_15_228,
  &str1_15_229,
  &str1_15_230,
  &str1_15_231,
  &str1_15_232,
  &str1_15_233,
  &str1_15_234,
  &str1_15_235,
  &str1_15_236,
  &str1_15_237,
  &str1_15_238,
  &str1_15_239,
  &str1_16_0,
  &str1_16_1,
  &str1_16_2,
  &str1_16_3,
  &str1_16_4,
  &str1_16_5,
  &str1_16_6,
  &str1_16_7,
  &str1_16_8,
  &str1_16_9,
  &str1_16_10,
  &str1_16_11,
  &str1_17_0,
  &str1_17_1,
  &str1_17_2,
  &str1_17_3,
  &str1_17_4,
  &str1_17_5,
  &str1_17_6,
  &str1_17_7,
  &str1_17_8,
  &str1_17_9,
  &str1_17_10,
  &str1_17_11,
  &str1_17_12,
  &str1_17_13,
  &str1_17_14,
  &str1_17_15,
  &str1_17_16,
  &str1_17_17,
  &str1_17_19,
  &str1_17_20,
  &str1_17_21,
  &str1_17_22,
  &str1_17_23,
  &str1_17_24,
// 1020
  &str1_14_142,
  &str1_2_54,
  &str1_2_55,
  &str1_2_56,
  &str1_5_142,
  &str1_5_143,
  &str1_5_144,
  &str1_5_145,
  &str1_5_146,
  &str1_5_147,
  &str1_5_148,
  &str1_15_241,
  &str1_7_135,
  &str1_7_137,
  &str1_7_138,
  &str1_7_139,
  &str1_7_140,
  &str1_7_141,
  &str1_7_142,
// 1039
  &str1_15_242,
  &str1_15_243,
  &str1_16_12,
  &str1_16_13,
  &str1_0_14,
  &str1_0_15,
  &str1_11_19,
  &str1_11_20,
  &str1_2_57,
  &str1_2_58,
  &str1_2_59,
  &str1_2_60,
  &str1_2_61,
  &str1_2_62,
  &str1_2_63,
  &str1_2_64,
  &str1_2_65,
  
  &str1_2_66,
  &str1_2_67,
  &str1_2_68,
  &str1_2_69,
  &str1_2_70,
  &str1_2_71,
  &str1_2_72,
  &str1_2_73,
  &str1_2_74,
  &str1_2_75,
  &str1_2_76,
  &str1_2_77,
  &str1_2_78,
  &str1_2_79,
  &str1_2_80,
  &str1_2_81,
  &str1_2_82,
  &str1_2_83,
  &str1_2_84,
  &str1_2_85,
  &str1_2_86,
  &str1_2_87,
  &str1_2_88,
  &str1_2_89,
  &str1_2_90,
  &str1_2_91,
  &str1_2_92,
  &str1_2_93,
  &str1_2_94,
  &str1_2_95,
  &str1_2_96,
  &str1_2_97,
  &str1_2_98,
  &str1_2_99,
  &str1_2_100,
  &str1_2_101,
  &str1_2_102,
  &str1_2_103,
  &str1_2_104,
  &str1_2_105,
  &str1_2_106,
  &str1_2_107,
  &str1_2_108,
  &str1_2_109,
  &str1_2_110,
  &str1_10_16,
  &str1_10_17,
  &str1_10_18,
  &str1_13_20,
  &str1_13_21,
  &str1_13_22,
  &str1_13_23,
  &str1_13_24,
  &str1_13_25,
  &str1_13_26,
  &str1_1_11,
  &str1_1_12,
  &str1_1_13,
  &str1_1_14,
  &str1_3_20,
  &str1_3_21,
  &str1_3_22,
  &str1_3_23,
  &str1_8_50,
  &str1_8_51,
  &str1_8_52,
  &str1_8_53,
  &str1_8_54,
  &str1_8_55,
  &str1_8_56,
  &str1_8_57,
  &str1_9_48,
  &str1_9_49,
  &str1_9_50,
  &str1_9_51,
  &str1_9_52,
  &str1_9_53,
  &str1_12_36,
  &str1_12_37,
  &str1_12_38,
  &str1_12_39,
  &str1_12_40,
  &str1_12_41,
  &str1_4_58,
  &str1_4_59,
  &str1_4_60,
  &str1_4_61,
  &str1_4_62,
  &str1_4_63,
  &str1_4_64,
  &str1_4_65,
  &str1_5_149,
  &str1_5_150,
  &str1_5_151,
  &str1_5_152,
  &str1_5_153,
  &str1_5_154,
  &str1_5_155,
  &str1_5_156,
  &str1_5_157,
  &str1_5_158,
  &str1_5_159,
  &str1_5_160,
  &str1_6_72,
  &str1_6_73,
  &str1_6_76,
  &str1_7_143,
  &str1_7_144,
  &str1_7_145,
  &str1_7_146,
  &str1_7_147,
  &str1_7_148,
  &str1_7_149,
  &str1_7_150,
  &str1_7_151,
  &str1_7_152,
  &str1_7_153,
  &str1_7_154,
  &str1_7_155,
  &str1_7_156,
  &str1_14_143,
  &str1_14_144,
  &str1_14_145,
  &str1_14_146,
  &str1_14_147,
  &str1_14_148,
  &str1_14_149,
  &str1_14_150,
  &str1_14_151,
  &str1_14_152,
  &str1_14_153,
  &str1_14_154,
  &str1_14_155,
  &str1_14_156,
  &str1_14_157,
  &str1_14_158,
  &str1_14_159,
  &str1_14_160,
  &str1_14_161,
  &str1_14_162,
  &str1_2_111,
  &str1_6_77,
  &str1_6_78,
  &str1_6_79,
  &str1_6_80,
  &str1_6_81,
  &str1_6_82,
  &str1_7_157,
  &str1_7_158,
  &str1_7_159,
};

const char **console_str1[CONSOLE_STRINGS];

