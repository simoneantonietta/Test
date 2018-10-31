#include "console.h"

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_attuatori.c */
const char *str2_0_0="ATN064041ATTUATORE\r";
const char *str2_0_1="ATN064051ESCLUSO\r";
const char *str2_0_2="ATN064071ESCLUDERE?\r";
const char *str2_0_3="ESCLUDI ATT.";
const char *str2_0_4="";//ATN064041ATTUATORE\r";
const char *str2_0_5="ATN064051INCLUSO\r";
const char *str2_0_6="ATN064071INCLUDERE?\r";
const char *str2_0_7="INCLUDI ATT.";
const char *str2_0_8="";//ATN064041ATTUATORE\r";
const char *str2_0_9="ATN064051ON\r";
const char *str2_0_10="ON ATTUATORE";
const char *str2_0_11="";//ATN064041ATTUATORE\r";
const char *str2_0_12="ATN064051OFF\r";
const char *str2_0_13="OFF ATTUATORE";
const char *str2_0_14="ATN064041%s\r";
const char *str2_0_15="ATN064051%2$s\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_badge.c */
const char *str2_1_0="ATN064041LETTORE\r";
const char *str2_1_1="ATN064051NON VALIDO\r";
const char *str2_1_2="ATN064041BADGE\r";
const char *str2_1_3="ATN064051ABILITATO\r";
const char *str2_1_4="ATN064051DISABILITATO\r";
const char *str2_1_5="ATN064041BADGE\r";
const char *str2_1_6="ATN064051NON VALIDO\r";
const char *str2_1_7="ATN064071NUM. LETTORE\r";
const char *str2_1_8="ATN064041NUM. BADGE\r";
const char *str2_1_9="ABILITA BADGE";
const char *str2_1_10="DISABILITA BADGE";
const char *str2_1_11="ATN056080___\r";
const char *str2_1_12="ATG05608\r";
const char *str2_1_13="ATN056050____\r";
const char *str2_1_14="ATG05605\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_impostazioni.c */
const char *str2_2_0="ATN064041FASCIA\r";
const char *str2_2_1="ATN064051MODIFICATA\r";
const char *str2_2_2="ATN064071ORA FINE\r";
const char *str2_2_3="ATN064031FASCIA %03d\r";
const char *str2_2_4="ATN064051ORA INIZIO\r";
const char *str2_2_5="FASCE ORARIE";
const char *str2_2_6="FASCIA %03d";
const char *str2_2_7="Feriale  ";
const char *str2_2_8="Semifest.";
const char *str2_2_9="Festivo  ";
const char *str2_2_10="ATN064041TIPO GIORNO\r";
const char *str2_2_11="ATN064051MODIFICATO\r";
const char *str2_2_12="FESTIVI";
const char *str2_2_13="ATN064041DATA\r";
const char *str2_2_14="ATN064051MODIFICATA\r";
const char *str2_2_15="ATN064041INSERIRE DATA\r";
const char *str2_2_16="ATN064041ORA\r";
const char *str2_2_17="ATN064051MODIFICATA\r";
const char *str2_2_18="ATN064041INSERIRE ORA\r";
const char *str2_2_19="ATN048060 DATA \r";
const char *str2_2_20="ATN048070 ORA   \r";
const char *str2_2_23="DATA E ORA";
const char *str2_2_24="ATN064041VARIARE?\r";
const char *str2_2_25="ATN064041ZONA\r";
const char *str2_2_26="ATN064051NON VALIDA\r";
const char *str2_2_27="";//Zona insieme (old) %d:";
const char *str2_2_28="";//  zona %d:";
const char *str2_2_29="";//Zona insieme (new) %d:";
const char *str2_2_30="";//  zona %d:";
const char *str2_2_31="";//Zona totale:";
const char *str2_2_32="";//  zona %d:";
const char *str2_2_33="ATN064051MODIFICATA\r";
const char *str2_2_34="ZONE/SENSORI";
const char *str2_2_35="ATN064031NUM. SENSORE?\r";
const char *str2_2_36="ATN064041ZONA\r";
const char *str2_2_37="ATN064051NON VALIDA\r";
const char *str2_2_38="ATN064051MODIFICATA\r";
const char *str2_2_39="ATN064031NUM. ZS?\r";
const char *str2_2_40="ATN064041RITARDO\r";
const char *str2_2_41="ATN064051NON VALIDO\r";
const char *str2_2_42="ATN064051IMPOSTATO\r";
const char *str2_2_43="ATN038070Rit:            s\r";
const char *str2_2_44="ZONE RITARDATE";
const char *str2_2_45="ATN064031NUM. ZS?\r";
const char *str2_2_46="ATN064041TIMEOUT\r";
const char *str2_2_47="ATN064051NON VALIDO\r";
const char *str2_2_48="ATN064041TIMEOUT\r";
const char *str2_2_49="ATN064051MODIFICATO\r";
const char *str2_2_50="OUT LOGIN";
const char *str2_2_51="ATN064041TEMPO (sec)?\r";
const char *str2_2_52="ATN066071ERRORE\r";
const char *str2_2_53="CODICI";
const char *str2_2_54="INDIRIZZO IP";
const char *str2_2_55="ATN064041Impostazioni\r";
const char *str2_2_56="ATN064051salvate.\r";
const char *str2_2_57="ATN064041%s\r";
const char *str2_2_58="ATN064051%s\r";
const char *str2_2_59="ATN031070ZS: ___\r";
const char *str2_2_60="ATG04807\r";
const char *str2_2_61="ATN033070ZI: ___\r";
const char *str2_2_62="ATG05807\r";
const char *str2_2_63="ATG05606\r";
const char *str2_2_64="ATN052070____\r";
const char *str2_2_65="ATG05207\r";
const char *str2_2_66="ATN000030IP:\r";
const char *str2_2_67="ATN000040___.      .      .\r";
const char *str2_2_68="ATG00004\r";
const char *str2_2_69="ATN000040%03d\r";
const char *str2_2_70="ATN018040.___.      .       \r";
const char *str2_2_71="ATG02104\r";
const char *str2_2_72="ATN021040%03d\r";
const char *str2_2_73="ATN039040.___.       \r";
const char *str2_2_74="ATG04204\r";
const char *str2_2_75="ATN042040%03d\r";
const char *str2_2_76="ATN060040.___ \r";
const char *str2_2_77="ATG06304\r";
const char *str2_2_78="ATN063040%03d \r";
const char *str2_2_79="ATN000050Netmask:\r";
const char *str2_2_80="ATN000060___.      .      .\r";
const char *str2_2_81="ATG00006\r";
const char *str2_2_82="ATN000060%03d\r";
const char *str2_2_83="ATN018060.___.      .       \r";
const char *str2_2_84="ATG02106\r";
const char *str2_2_85="ATN021060%03d\r";
const char *str2_2_86="ATN039060.___.       \r";
const char *str2_2_87="ATG04206\r";
const char *str2_2_88="ATN042060%03d\r";
const char *str2_2_89="ATN060060.___ \r";
const char *str2_2_90="ATG06306\r";
const char *str2_2_91="ATN063060%03d \r";
const char *str2_2_92="ATN000070Gateway:\r";
const char *str2_2_93="ATN000080___.      .      .\r";
const char *str2_2_94="ATG00008\r";
const char *str2_2_95="ATN000080%03d\r";
const char *str2_2_96="ATN018080.___.      .       \r";
const char *str2_2_97="ATG02108\r";
const char *str2_2_98="ATN021080%03d\r";
const char *str2_2_99="ATN039080.___.       \r";
const char *str2_2_100="ATG04208\r";
const char *str2_2_101="ATN042080%03d\r";
const char *str2_2_102="ATN060080.___ \r";
const char *str2_2_103="ATG06308\r";
const char *str2_2_104="ATN063080%03d \r";
const char *str2_2_105="ATN000030IP:\r";
const char *str2_2_106="ATN000040%s\r";
const char *str2_2_107="ATN000050Netmask:\r";
const char *str2_2_108="ATN000060%s\r";
const char *str2_2_109="ATN000070Gateway:\r";
const char *str2_2_110="ATN000080%s\r";
const char *str2_2_111="ATN064061DHCP attivo\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_inoltra.c */
const char *str2_3_0="ATN064041SEGRETO\r";
const char *str2_3_1="ATN064051NON VALIDO\r";
const char *str2_3_2="ATN064041ERRORE\r";
const char *str2_3_3="ATN064051IMPOSTAZIONE\r";
const char *str2_3_4="";//ATN064041SEGRETO\r";
const char *str2_3_5="ATN064051INVIATO\r";
const char *str2_3_6="ATN064041TIPO SEGRETO\r";
const char *str2_3_7="";//ATN064051NON VALIDO\r";
const char *str2_3_8="ATN064071SEGRETO NUMERO\r";
const char *str2_3_9="INVIA SEGRETO";
const char *str2_3_10="ATN064041TIPO SEGRETO\r";
const char *str2_3_11="ATN064041EVENTO\r";
const char *str2_3_12="";//ATN064051NON VALIDO\r";
const char *str2_3_13="";//ATN064041EVENTO\r";
const char *str2_3_14="";//ATN064051INVIATO\r";
const char *str2_3_15="ATN064041EVENTO NUMERO\r";
const char *str2_3_16="";//ATN064051NON VALIDO\r";
const char *str2_3_17="ATN064071PARAMETRO\r";
const char *str2_3_18="INVIA EVENTO";
const char *str2_3_19="ATN064041EVENTO NUMERO\r";
const char *str2_3_20="ATN050080_____\r";
const char *str2_3_21="ATG05008\r";
const char *str2_3_22="ATN060050_\r";
const char *str2_3_23="ATG06005\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_lara_impostazioni.c */
const char *str2_4_0="ATN064051Domenica\r";
const char *str2_4_1="ATN064051Lunedì\r";
const char *str2_4_2="ATN064051Martedì\r";
const char *str2_4_3="ATN064051Mercoledì\r";
const char *str2_4_4="ATN064051Giovedì\r";
const char *str2_4_5="ATN064051Venerdì\r";
const char *str2_4_6="ATN064051Sabato\r";
const char *str2_4_7="ATN064041Fascia sett. N%02d\r";
const char *str2_4_8="Lunedì";
const char *str2_4_9="Martedì";
const char *str2_4_10="Mercoledì";
const char *str2_4_11="Giovedì";
const char *str2_4_12="Venerdì";
const char *str2_4_13="Sabato";
const char *str2_4_14="Domenica";
const char *str2_4_15="ATN064061(Nessuna)\r";
const char *str2_4_16="ATN064071VUOTO\r";
const char *str2_4_17="ATN064061SINGOLO VARIAB.\r";
const char *str2_4_18="ATN064061PERIODO VARIAB.\r";
const char *str2_4_19="ATN034070da %02d/%02d/%02d\r";
const char *str2_4_20="ATN040080a %02d/%02d/%02d\r";
const char *str2_4_21="ATN064061SINGOLO FISSO\r";
const char *str2_4_22="ATN064061PERIODO FISSO\r";
const char *str2_4_23="ATN043070da %02d/%02d\r";
const char *str2_4_24="ATN049080a %02d/%02d\r";
const char *str2_4_25="ATN064041FEST. ASSOCIATE\r";
const char *str2_4_33="ATN064031Fascia sett. N%02d\r";
const char *str2_4_41="FASCE SETTIMAN.";
const char *str2_4_42="ATN064051i terminali?\r";
const char *str2_4_43="ATN064041 a tutti \r";
const char *str2_4_44="ATN064051    i profili?    \r";
const char *str2_4_45="ATN064031Aggiungere\r";
const char *str2_4_46="ATN064041a tutte\r";
const char *str2_4_47="ATN064051le tessere?\r";
const char *str2_4_48="ATN019070 ENTER \r";
const char *str2_4_49="ATO Si  \r";
const char *str2_4_50="ATO DEL \r";
const char *str2_4_51="ATO No\r";
const char *str2_4_52="Cancella";
const char *str2_4_53="Singolo variab.";
const char *str2_4_54="Periodo variab.";
const char *str2_4_55="Singolo fisso";
const char *str2_4_56="Periodo fisso";
const char *str2_4_57="FESTIVITA'";
const char *str2_4_58="ATN064041%s\r";
const char *str2_4_59="ATN064051%s\r";
const char *str2_4_60="ATN040070%02d/%02d/%02d\r";
const char *str2_4_61="ATN048070%02d/%02d\r";
const char *str2_4_62="ATN040070  -:-  \r";
const char *str2_4_63="ATN088070  -:-  \r";
const char *str2_4_64="ATN040080  -:-  \r";
const char *str2_4_65="ATN088080  -:-  \r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_lara_pass.c */
const char *str2_5_0="BADGE/CNT";
const char *str2_5_1="PROFILO";
const char *str2_5_2="AREA";
const char *str2_5_3="STATO PASS";
const char *str2_5_4="BADGE/PIN";
const char *str2_5_5="";//PROFILO";
const char *str2_5_6="";//AREA";
const char *str2_5_7="";//STATO PASS";
const char *str2_5_8="";//BADGE/CNT";
const char *str2_5_9="";//PROFILO";
const char *str2_5_10="";//AREA";
const char *str2_5_11="";//STATO PASS";
const char *str2_5_12="SUPERVISOR";
const char *str2_5_13="";//BADGE/PIN";
const char *str2_5_14="";//PROFILO";
const char *str2_5_15="";//AREA";
const char *str2_5_16="";//STATO PASS";
const char *str2_5_17="";//SUPERVISOR";
const char *str2_5_18="TERMINALI";
const char *str2_5_19="FASCE SETTIMAN.";
const char *str2_5_20="FESTIVITA'";
const char *str2_5_21="OPZIONI";
const char *str2_5_22="";//TERMINALI";
const char *str2_5_23="";//FASCE SETTIMAN.";
const char *str2_5_24="";//FESTIVITA'";
const char *str2_5_25="";//OPZIONI";
const char *str2_5_26="ATS04\r";
const char *str2_5_27="ATN064051BADGE PRESENTE\r";
const char *str2_5_28="ATN064051BADGE ASSENTE\r";
const char *str2_5_29="ATN064071VALORE CONTATORE\r";
const char *str2_5_30="";//ATN0105BADGE PRESENTE\r";
const char *str2_5_31="";//ATN0105BADGE ASSENTE\r";
const char *str2_5_32="ATN064071PIN PRESENTE\r";
const char *str2_5_33="ATN064071PIN ASSENTE\r";
const char *str2_5_34="ATN064041TERM. ASSOCIATI\r";
const char *str2_5_35="ATN064061ANTI-COERCIZIONE\r";
const char *str2_5_36="ATN064071ATTIVATO\r";
const char *str2_5_37="ATN064071DISATTIVATO\r";
const char *str2_5_38="ATN064061ANTI-PASSBACK\r";
const char *str2_5_39="";//ATN0307DISATTIVATO\r";
const char *str2_5_40="";//ATN0407ATTIVATO\r";
const char *str2_5_41="ATN064041OPZIONI\r";
const char *str2_5_42="COERCIZIONE";
const char *str2_5_43="PASS-BACK";
const char *str2_5_44="ATN064051STATO ABIL\r";
const char *str2_5_45="ATN064071ABILITATO\r";
const char *str2_5_46="ATN064071DISABILITATO\r";
const char *str2_5_47="ATN064051SCONOSCIUTO\r";
const char *str2_5_48="PASS";
const char *str2_5_49="ATN064041ID NUM.\r";
const char *str2_5_50="ATN064061ERRATO\r";
const char *str2_5_51="ATN064061ASSEGNATO\r";
const char *str2_5_52="ATN064071CONFERMA PIN\r";
const char *str2_5_53="ATN064061ERRORE\r";
const char *str2_5_54="ATN064051NUOVO PIN\r";
const char *str2_5_55="ATN064081(max 5 cifre)\r";
const char *str2_5_56="ATN064051VECCHIO PIN\r";
const char *str2_5_57="ATN064051NUOVO VALORE\r";
const char *str2_5_58="";//ATN0406ASSEGNATO\r";
const char *str2_5_59="ATN064051CONTATORE\r";
const char *str2_5_60="ATN064071NUOVO VALORE\r";
const char *str2_5_61="ATN064051TERMINALE\r";
const char *str2_5_62="";//ATN0406ASSEGNATO\r";
const char *str2_5_63="ATN064041ASSEGNA\r";
const char *str2_5_64="";//ATN0405TERMINALE\r";
const char *str2_5_65="ATN064061INIBITO\r";
const char *str2_5_66="ATN064041INIBISCI\r";
const char *str2_5_67="ASSEGNA";
const char *str2_5_68="INIBISCI";
const char *str2_5_69="ATN064041TERMINALI\r";
const char *str2_5_70="ATN064051FASCIA\r";
const char *str2_5_71="ATN064061ASSOCIATA\r";
const char *str2_5_72="ATN064041ASSOCIA\r";
const char *str2_5_73="";//ATN0605FASCIA\r";
const char *str2_5_74="ATN064061ANNULLATA\r";
const char *str2_5_75="ATN064041ANNULLA\r";
const char *str2_5_76="ATN064061(Nessuna)\r";
const char *str2_5_77="ASSOCIA";
const char *str2_5_78="ANNULLA";
const char *str2_5_79="VISUALIZZA";
const char *str2_5_80="ATN064041FASCE SETTIM.\r";
const char *str2_5_81="ATN064051FESTIVITA'\r";
const char *str2_5_82="";//ATN0406ASSOCIATA\r";
const char *str2_5_83="";//ATN0504ASSOCIA\r";
const char *str2_5_84="";//ATN0405FESTIVITA'\r";
const char *str2_5_85="";//ATN0406ANNULLATA\r";
const char *str2_5_86="";//ATN0504ANNULLA\r";
const char *str2_5_87="ASSOCIA";
const char *str2_5_88="ANNULLA";
const char *str2_5_89="VISUALIZZA";
const char *str2_5_90="ATN064041FESTIVITA'\r";
const char *str2_5_91="ATN064061ATTIVATO\r";
const char *str2_5_92="ATN064061DISATTIVATO\r";
const char *str2_5_93="ATN064051ANTI-COERCIZIONE\r";
const char *str2_5_94="";//ATN0506ATTIVATO\r";
const char *str2_5_95="ATN064081DISATTIVARE?\r";
const char *str2_5_96="ATN064061DISATTIVATO\r";
const char *str2_5_97="ATN064081ATTIVARE?\r";
const char *str2_5_98="";//ATN0306DISATTIVATO\r";
const char *str2_5_99="";//ATN0506ATTIVATO\r";
const char *str2_5_100="ATN064051ANTI-PASSBACK\r";
const char *str2_5_101="";//ATN0506ATTIVATO\r";
const char *str2_5_102="";//ATN0308DISATTIVARE?\r";
const char *str2_5_103="";//ATN0306DISATTIVATO\r";
const char *str2_5_104="";//ATN0508ATTIVARE?\r";
const char *str2_5_105="";//ATN0306DISATTIVATO\r";
const char *str2_5_106="";//ATN0506ATTIVATO\r";
const char *str2_5_107="ATN064051SUPERVISOR\r";
const char *str2_5_108="";//ATN0506ATTIVATO\r";
const char *str2_5_109="";//ATN0308DISATTIVARE?\r";
const char *str2_5_110="";//ATN0306DISATTIVATO\r";
const char *str2_5_111="";//ATN0508ATTIVARE?\r";
const char *str2_5_112="";//COERCIZIONE";
const char *str2_5_113="";//PASS-BACK";
const char *str2_5_114="";//SUPERVISOR";
const char *str2_5_115="ATN064041OPZIONI\r";
const char *str2_5_116="ATN064051NUOVO PROFILO\r";
const char *str2_5_117="ATN064061ASSEGNATO\r";
const char *str2_5_118="ATN064051AREA PRESENZA\r";
const char *str2_5_119="ATN064061VARIATA\r";
const char *str2_5_120="ATN064051ABILITATO\r";
const char *str2_5_121="ATN064051DISABILITATO\r";
const char *str2_5_122="ATN064071CONFERMA?\r";
const char *str2_5_123="ATN064051CANCELLATO\r";
const char *str2_5_124="";//ATN0507CONFERMA?\r";
const char *str2_5_125="DISABILITA";
const char *str2_5_126="CANCELLA";
const char *str2_5_127="ABILITA";
const char *str2_5_128="CANCELLA";
const char *str2_5_129="ATN064041STATO ABIL\r";
const char *str2_5_130="";//ATN0405ABILITATO\r";
const char *str2_5_131="";//ATN0305DISABILITATO\r";
const char *str2_5_132="";//ATN0306DISATTIVATO\r";
const char *str2_5_133="";//ATN0506ATTIVATO\r";
const char *str2_5_134="ATN064051SUPERVISOR\r";
const char *str2_5_135="";//ATN0506ATTIVATO\r";
const char *str2_5_136="";//ATN0308DISATTIVARE?\r";
const char *str2_5_137="";//ATN0306DISATTIVATO\r";
const char *str2_5_138="";//ATN0508ATTIVARE?\r";
const char *str2_5_139="ATN064051SCONOSCIUTO\r";
const char *str2_5_140="PASS";
const char *str2_5_141="ATN064041ID NUM.\r";
const char *str2_5_142="PRIMARIO";
const char *str2_5_143="ASSEGNA SECOND.";
const char *str2_5_144="CANCELLA SECOND.";
const char *str2_5_145="ATN064041Profilo\r";
const char *str2_5_146="ATN064051completo\r";
const char *str2_5_147="ATN064051aggiunto\r";
const char *str2_5_148="ATN064051eliminato\r";
const char *str2_5_149="ATN064081%06d\r";
const char *str2_5_150="ATN064051%s\r";
const char *str2_5_151="ATN064061%s\r";
const char *str2_5_152="ATN064061%s\r";
const char *str2_5_153="ATN064071%s\r";
const char *str2_5_154="ATN064031ID %05d\r";
const char *str2_5_155="ATG04806\r";
const char *str2_5_156="ATG04808\r";
const char *str2_5_157="ATN064061%05d\r";
const char *str2_5_158="ATN064041%s\r";
const char *str2_5_159="ATN064051%s\r";
const char *str2_5_160="ATN064041%s\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_lara_profilo.c */
const char *str2_6_0="TERMINALI";
const char *str2_6_1="FASCIA SETTIM.";
const char *str2_6_2="FESTIVITA'";
const char *str2_6_3="OPZIONI";
const char *str2_6_4="";//TERMINALI";
const char *str2_6_5="";//FASCIA SETTIM.";
const char *str2_6_6="";//FESTIVITA'";
const char *str2_6_7="";//OPZIONI";
const char *str2_6_8="ATN064041TERM. ASSOCIATI\r";
const char *str2_6_9="ATN064051FASCIA ASSOCIATA\r";
const char *str2_6_10="ATN064071Nessuna\r";
const char *str2_6_11="ATN064051ANTI-COERCIZIONE\r";
const char *str2_6_12="ATN064061ATTIVATO\r";
const char *str2_6_13="ATN064061DISATTIVATO\r";
const char *str2_6_14="ATN064051ANTI-PASSBACK\r";
const char *str2_6_15="";//ATN0306DISATTIVATO\r";
const char *str2_6_16="";//ATN0506ATTIVATO\r";
const char *str2_6_17="COERCIZIONE";
const char *str2_6_18="PASS BACK";
const char *str2_6_19="ATN064041OPZIONI\r";
const char *str2_6_20="PROFILI";
const char *str2_6_21="ATN064051TERMINALE\r";
const char *str2_6_22="ATN064061ASSEGNATO\r";
const char *str2_6_23="ATN064041ASSEGNA\r";
const char *str2_6_24="";//ATN0405TERMINALE\r";
const char *str2_6_25="ATN064061INIBITO\r";
const char *str2_6_26="ATN064041INIBISCI\r";
const char *str2_6_27="ASSEGNA";
const char *str2_6_28="INIBISCI";
const char *str2_6_29="ATN064041TERMINALI\r";
const char *str2_6_30="ATN064051FASCIA\r";
const char *str2_6_31="ATN064061ASSOCIATA\r";
const char *str2_6_32="ATN064041ASSOCIA\r";
const char *str2_6_33="ATN064051FASCIA\r";
const char *str2_6_34="ATN064061ANNULLATA\r";
const char *str2_6_35="ATN064041ANNULLA\r";
const char *str2_6_36="ATN064061(Nessuna)\r";
const char *str2_6_37="ASSOCIA";
const char *str2_6_38="ANNULLA";
const char *str2_6_39="VISUALIZZA";
const char *str2_6_40="ATN064041FASCE SETTIM.\r";
const char *str2_6_41="ATS04\r";
const char *str2_6_42="ATN064051FESTIVITA'\r";
const char *str2_6_43="";//ATN0406ASSOCIATA\r";
const char *str2_6_44="";//ATN0504ASSOCIA\r";
const char *str2_6_45="";//ATN0405FESTIVITA'\r";
const char *str2_6_46="";//ATN0406ANNULLATA\r";
const char *str2_6_47="";//ATN0504ANNULLA\r";
const char *str2_6_48="";//ASSOCIA";
const char *str2_6_49="";//ANNULLA";
const char *str2_6_50="";//VISUALIZZA";
const char *str2_6_51="ATN064041FESTIVITA'\r";
const char *str2_6_52="";//ATN0506ATTIVATO\r";
const char *str2_6_53="";//ATN0306DISATTIVATO\r";
const char *str2_6_54="ATN064051ANTI-COERCIZIONE\r";
const char *str2_6_55="";//ATN0506ATTIVATO\r";
const char *str2_6_56="ATN064081DISATTIVARE?\r";
const char *str2_6_57="";//ATN0306DISATTIVATO\r";
const char *str2_6_58="ATN064081ATTIVARE?\r";
const char *str2_6_59="";//ATN0306DISATTIVATO\r";
const char *str2_6_60="";//ATN0506ATTIVATO\r";
const char *str2_6_61="ATN064051ANTI-PASSBACK\r";
const char *str2_6_62="";//ATN0506ATTIVATO\r";
const char *str2_6_63="";//ATN0308DISATTIVARE?\r";
const char *str2_6_64="";//ATN0306DISATTIVATO\r";
const char *str2_6_65="";//ATN0508ATTIVARE?\r";
const char *str2_6_66="";//COERCIZIONE";
const char *str2_6_67="";//PASS-BACK";
const char *str2_6_68="";//ATN0504OPZIONI\r";
const char *str2_6_69="ATN064031%s\r";
const char *str2_6_70="ATN064041%s\r";
const char *str2_6_71="";//PROFILI";
const char *str2_6_72="ATN064071%s\r";
const char *str2_6_73="ATN064081%s\r";
const char *str2_6_76="ATN064061%s\r";
const char *str2_6_77="TIPO\r";
const char *str2_6_78="ATN064051TIPO:\r";
const char *str2_6_79="ATN064061NORMALE\r";
const char *str2_6_80="ATN064061SCORTA\r";
const char *str2_6_81="ATN064061VISITATORE (O)\r";
const char *str2_6_82="ATN064061VISITATORE (A)\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_lara_term.c */
const char *str2_7_0="CONTATORE";
const char *str2_7_1="FASCIA SETTIM.";
const char *str2_7_2="FESTIVITA'";
const char *str2_7_3="DIN";
const char *str2_7_4="";//FASCIA SETTIM.";
const char *str2_7_5="";//FESTIVITA'";
const char *str2_7_6="";//CONTATORE";
const char *str2_7_7="";//FASCIA SETTIM.";
const char *str2_7_8="";//FESTIVITA'";
const char *str2_7_9="AREA ENTRATA";
const char *str2_7_10="AREA USCITA";
const char *str2_7_11="TEMPI";
const char *str2_7_12="OPZIONI";
const char *str2_7_13="";//DIN";
const char *str2_7_14="";//FASCIA SETTIM.";
const char *str2_7_15="";//FESTIVITA'";
const char *str2_7_16="";//AREA ENTRATA";
const char *str2_7_17="";//AREA USCITA";
const char *str2_7_18="";//TEMPI";
const char *str2_7_19="";//OPZIONI";
const char *str2_7_20="ATN064061CONTATORE\r";
const char *str2_7_21="ATN064071DECREMENTO\r";
const char *str2_7_22="ATN064071NO DECREMENTO\r";
const char *str2_7_23="ATN064071PRESENTE\r";
const char *str2_7_24="ATN064071ASSENTE\r";
const char *str2_7_25="ATN064051FASCIA ASSOCIATA\r";
const char *str2_7_26="ATN064071Nessuna\r";
const char *str2_7_27="TERMINALI";
const char *str2_7_29="ATN064061ERRATO\r";
const char *str2_7_30="ATN064061ASSEGNATO\r";
const char *str2_7_31="ATN064051DECREMENTO\r";
const char *str2_7_33="ATN064061ERRATO\r";
const char *str2_7_34="ATN064061ASSEGNATO\r";
const char *str2_7_35="ATN064071CONFERMA DIN\r";
const char *str2_7_36="ATN064051NUOVO DIN\r";
const char *str2_7_37="ATN064081(max 5 cifre)\r";
const char *str2_7_38="ATN064051FASCIA\r";
const char *str2_7_39="ATN064061ASSOCIATA\r";
const char *str2_7_40="ATN064041ASSOCIA\r";
const char *str2_7_41="ATN064051FASCIA\r";
const char *str2_7_42="ATN064061ANNULLATA\r";
const char *str2_7_43="ATN064041ANNULLA\r";
const char *str2_7_44="ATN064061(Nessuna)\r";
const char *str2_7_45="ASSOCIA";
const char *str2_7_46="ANNULLA";
const char *str2_7_47="VISUALIZZA";
const char *str2_7_48="ATN064041FASCE SETTIM.\r";
const char *str2_7_49="ATN064051FESTIVITA'\r";
const char *str2_7_50="ATN064061ASSOCIATA\r";
const char *str2_7_51="ATN064041ASSOCIA\r";
const char *str2_7_52="ATN064051FESTIVITA'\r";
const char *str2_7_53="ATN064061ANNULLATA\r";
const char *str2_7_54="ATN064041ANNULLA\r";
const char *str2_7_55="ASSOCIA";
const char *str2_7_56="ANNULLA";
const char *str2_7_57="VISUALIZZA";
const char *str2_7_58="ATN064041FESTIVITA'\r";
const char *str2_7_59="ATN064051AREA ENTRATA\r";
const char *str2_7_60="ATN064061VARIATA\r";
const char *str2_7_61="ATN064051AREA USCITA\r";
const char *str2_7_62="ATN064061VARIATA\r";
const char *str2_7_63="ATN064061VARIA AREA\r";
const char *str2_7_64="ATN064041ENTRATA\r";
const char *str2_7_65="ATN064041USCITA\r";
const char *str2_7_66="ATN064071ERRORE\r";
const char *str2_7_67="ATN064061VARIATO\r";
const char *str2_7_68="ATN064041TEMPO\r";
const char *str2_7_69="ATN064051ANTI PASSBACK\r";
const char *str2_7_70="ATN064061%02d min\r";
const char *str2_7_71="ATN064071VARIA TEMPO\r";
const char *str2_7_72="ATN064071ERRORE\r";
const char *str2_7_73="ATN064061VARIATO\r";
const char *str2_7_74="ATN064041TEMPO\r";
const char *str2_7_75="ATN064051APERT. PROLUNG.\r";
const char *str2_7_76="ATN064061%03d sec\r";
const char *str2_7_77="ATN064071VARIA TEMPO\r";
const char *str2_7_81="ATN064071ERRORE\r";
const char *str2_7_82="ATN064061VARIATO\r";
const char *str2_7_83="ATN064041TEMPO\r";
const char *str2_7_84="ATN064051ON RELE' PORTA\r";
const char *str2_7_85="ATN064061%03d sec\r";
const char *str2_7_86="ATN064071VARIA TEMPO\r";
const char *str2_7_87="PASS BACK";
const char *str2_7_88="APERT. PROLUNG.";
const char *str2_7_89="ON RELE' PORTA";
const char *str2_7_90="ATN064041TEMPI\r";
const char *str2_7_91="ATN064061DISATTIVATO\r";
const char *str2_7_92="ATN064061ATTIVATO\r";
const char *str2_7_93="ATN064041ANTI PASSBACK\r";
const char *str2_7_94="ATN064051ATTIVATO\r";
const char *str2_7_95="ATN064071DISATTIVARE?\r";
const char *str2_7_96="ATN064051DISATTIVATO\r";
const char *str2_7_97="ATN064071ATTIVARE?\r";
const char *str2_7_98="";// "ATN064061SLAVE\r";
const char *str2_7_99="";// "ATN064071PROGRAMMATO\r";
const char *str2_7_100="";// "ATN064071ANNULLATO\r";
const char *str2_7_101="";// "ATN064041MASTER\r";
const char *str2_7_102="";// "ATN064061SLAVE\r";
const char *str2_7_103="";// "ATN064081CONFERMA?\r";
//const char *str2_7_104="ATN064041MASTER/SLAVE\r";
const char *str2_7_104="ATN064041BLOCCO TIMBR.\r";
const char *str2_7_105="";// "ATN064051MASTER\r";
const char *str2_7_106="";// "ATN064071ANNULLA SLAVE?\r";
const char *str2_7_107="";// "ATN064051NO SLAVE\r";
const char *str2_7_108="";// "ATN064071PROGRAMMA SLAVE?\r";
const char *str2_7_109="ATN064071BLOCCATI\r";
const char *str2_7_110="ATN064071SBLOCCATI\r";
const char *str2_7_111="ATN064041BLOCCO TASTI\r";
const char *str2_7_112="ATN064051BLOCCATI\r";
const char *str2_7_113="ATN064071SBLOCCARE?\r";
const char *str2_7_114="ATN064051SBLOCCATI\r";
const char *str2_7_115="ATN064071BLOCCARE?\r";
const char *str2_7_116="ATN064061SELEZIONATO\r";
const char *str2_7_117="ATN064061SELEZIONATO\r";
const char *str2_7_118="ATN064051BADGE+PIN\r";
const char *str2_7_119="ATN064051BADGE\r";
const char *str2_7_120="ATN064051PIN\r";
const char *str2_7_121="  BADGE+PIN";
const char *str2_7_122="";//  "  BADGE+PIN";
const char *str2_7_123="  BADGE";
const char *str2_7_124="";//  "  BADGE";
const char *str2_7_125="  PIN";
const char *str2_7_126="";//  "  PIN";
const char *str2_7_127="ATN064041FILTRO ENTRATA\r";
const char *str2_7_128="ATN064041FILTRO USCITA\r";
const char *str2_7_129="PASS BACK";
//const char *str2_7_130="MASTER/SLAVE";
const char *str2_7_130="BLOCCO TIMBR.";
const char *str2_7_131="BLOCCO TASTI";
const char *str2_7_132="FILTRO ENTRATA";
const char *str2_7_133="FILTRO USCITA";
const char *str2_7_134="ATN064041OPZIONI\r";
const char *str2_7_135="BIOMETRICO";
const char *str2_7_136="";//  "TERMINALI";
const char *str2_7_137="ATN064031Riconoscimento\r";
const char *str2_7_138="ATN064041biometrico volto\r";
const char *str2_7_139="ATN064051ATTIVO\r";
const char *str2_7_140="ATN064071DISATTIVARE?\r";
const char *str2_7_141="ATN064051DISATTIVO\r";
const char *str2_7_142="ATN064071ATTIVARE?\r";
const char *str2_7_143="ATN064081%06d\r";
const char *str2_7_144="ATN064061DIN\r";
const char *str2_7_145="ATN064071%s\r";
const char *str2_7_146="ATN064081%s\r";
const char *str2_7_147="ATN064031%s\r";
const char *str2_7_148="ATN064041%s\r";
const char *str2_7_149="ATN064061%06d\r";
const char *str2_7_150="ATN064071NUOVO VALORE\r";
const char *str2_7_151="ATG04808\r";
const char *str2_7_152="ATG04806\r";
const char *str2_7_153="ATN064061%s\r";
const char *str2_7_154="ATN064041%s\r";
const char *str2_7_155="ATN064051%s\r";
const char *str2_7_156="ATG06208\r";
const char *str2_7_157="ACC.A.RISERVATA";
const char *str2_7_158="ATN064031Controllo accesso\r";
const char *str2_7_159="ATN064041area riservata\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_liste.c */
const char *str2_8_0="ATN064041ALLARME\r";
const char *str2_8_1="ATN064051ACCETTATO\r";
const char *str2_8_2="ATN064041GUASTO\r";
const char *str2_8_3="";//ATN0405ACCETTATO\r";
const char *str2_8_4="ATN064041MANOMISSIONE\r";
const char *str2_8_5="ATN064051ACCETTATA\r";
const char *str2_8_6="ATN064041SENSORE\r";
const char *str2_8_7="ATN064051FUORI SERVIZIO\r";
const char *str2_8_8="ATN00007  ACCETTA\t\r";
const char *str2_8_9="ATN00008  FUORI SERVIZIO\t\r";
const char *str2_8_10="";//ATN0107ACCETTA\r";
const char *str2_8_11="";//ATN0108FUORI SERVIZIO\r";
const char *str2_8_12="";//ATN0108FUORI SERVIZIO\r";
const char *str2_8_14="SENS. in ALLARME";
const char *str2_8_15="SENS. da ACCETT.";
const char *str2_8_16="";//ATN064041SENSORE\r";
const char *str2_8_17="ATN064051INCLUSO\r";
const char *str2_8_18="ATN064041INCLUDERE\r";
const char *str2_8_19="SENSORI ESCLUSI";
const char *str2_8_20="SENSORI GUASTI";
const char *str2_8_21="SENS. MANOMESSI";
const char *str2_8_22="ATN064041ZONA\r";
const char *str2_8_23="ATN064051DISATTIVATA\r";
const char *str2_8_24="ATN064041ZONA %s\r";
const char *str2_8_25="ATN064061DISATTIVARE?\r";
const char *str2_8_26="ZONE ATTIVATE";
const char *str2_8_27="ZONA %s";
const char *str2_8_28="";//ATN0704ZONA\r";
const char *str2_8_29="ATN064051ATTIVATA\r";
const char *str2_8_30="ATN064041ZONA %s\r";
const char *str2_8_31="ATN064061ATTIVARE?\r";
const char *str2_8_32="ZONE DISATTIVATE";
const char *str2_8_33="ZONA %s";
const char *str2_8_34="ATN064041ATTUATORE\r";
const char *str2_8_35="ATN064051OFF\r";
const char *str2_8_36="ATN064041DISATTIVARE\r";
const char *str2_8_38="ATTUATORI ON";
const char *str2_8_39="";//ATN0404ATTUATORE\r";
const char *str2_8_40="ATN064051INCLUSO\r";
const char *str2_8_41="ATN064041INCLUDERE\r";
const char *str2_8_42="ATT. ESCLUSI";
const char *str2_8_43="Feriale  ";
const char *str2_8_44="Semifest.";
const char *str2_8_45="Festivo  ";
const char *str2_8_46="ATN064041TIPO GIORNO\r";
const char *str2_8_47="ATN064051MODIFICATO\r";
const char *str2_8_48="FESTIVI";
const char *str2_8_49="NUMERI CHIAMATI";
const char *str2_8_50="ATN000030%s\r";
const char *str2_8_51="ATN000040%s\r";
const char *str2_8_52="ATN000050%s\r";
const char *str2_8_53="ATN064051%s?\r";
const char *str2_8_54="ATN064071%s\r";
const char *str2_8_55="ATN064051%2$s\r";
const char *str2_8_56="ATN064051%s?\r";
const char *str2_8_57="ATN040%02d0 %s\t\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_modem.c */
const char *str2_9_0="ATN064041RUBRICA\r";
const char *str2_9_1="ATN064051COMPLETA\r";
const char *str2_9_2="ATN064041NUMERO SALVATO\r";
const char *str2_9_3="ATN064071NUMERO\r";
const char *str2_9_4="";//ATN0504RUBRICA\r";
const char *str2_9_5="";//ATN0505COMPLETA\r";
const char *str2_9_6="CARICA NUMERO";
const char *str2_9_7="ATN064041NOME TITOLARE\r";
const char *str2_9_8="ATN064041NUMERO\r";
const char *str2_9_9="ATN064051CANCELLATO\r";
const char *str2_9_10="CANCELLA NUMERO";
const char *str2_9_11="";//ATN0604NUMERO\r";
const char *str2_9_12="ATN064051ABILITATO\r";
const char *str2_9_13="Modem in uscita";
const char *str2_9_14="Modem in entrata";
const char *str2_9_15="SMS in uscita";
const char *str2_9_16="SMS in entrata";
const char *str2_9_17="GSM in uscita";
const char *str2_9_18="GSM in entrata";
const char *str2_9_19="ABILITA NUMERO";
const char *str2_9_20="";//ATN0604NUMERO\r";
const char *str2_9_21="ATN064051DISABILITATO\r";
const char *str2_9_28="DISABIL. NUMERO";
const char *str2_9_29="ATN064041Messaggio n.%d\r";
const char *str2_9_30="ATN064051registrato.\r";
const char *str2_9_31="ATN064071Durata: %ds\r";
const char *str2_9_32="ATN064041Premere INVIO\r";
const char *str2_9_33="";
const char *str2_9_34="ATN064051per terminare la\r";
const char *str2_9_35="ATN064061registrazione\r";
const char *str2_9_36="ATN064041Altra\r";
const char *str2_9_37="ATN064051registrazione\r";
const char *str2_9_38="ATN064061in corso.\r";
const char *str2_9_39="ATN064031Chiamare la\r";
const char *str2_9_40="ATN064041centrale.\r";
const char *str2_9_41="ATN064061Premere INVIO\r";
const char *str2_9_42="";//ATOINVIO\r";
const char *str2_9_43="ATN064071per iniziare la\r";
const char *str2_9_44="ATN064081registrazione\r";
const char *str2_9_45="MESSAGGI AUDIO";
const char *str2_9_46="Messaggio n.%d";
const char *str2_9_47="Nuovo messaggio";
const char *str2_9_48="ATN000080________________\r";
const char *str2_9_49="ATG00008\r";
const char *str2_9_50="ATN000050________________\r";
const char *str2_9_51="ATG00005\r";
const char *str2_9_52="ATN064031%s\r";
const char *str2_9_53="ATN064041%s\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_personalizza.c */
const char *str2_10_0="ATN064041DESCRIZIONE\r";
const char *str2_10_1="ATN064051MODIFICATA\r";
const char *str2_10_2="ATN064081(max 16 caratt.)\r";
const char *str2_10_3="PERS. ZONE";
const char *str2_10_4="ZONA %s";
const char *str2_10_5="PERS. SENSORI";
const char *str2_10_6="PERS. ATTUATORI";
const char *str2_10_7="PERS.TELECOMANDI";
const char *str2_10_9="PERS. TITOLARI";
const char *str2_10_10="PERS. CODICI";
const char *str2_10_11="PERS. PROFILI";
const char *str2_10_12="PERS. AREA";
const char *str2_10_13="PERS. FESTIVITA'";
const char *str2_10_14="PERS. FO SETT.";
const char *str2_10_15="PERS. TERMINALI";
const char *str2_10_16="ATN000040%s\r";
const char *str2_10_17="ATN000060________________\r";
const char *str2_10_18="ATG00006\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_sensori.c */
const char *str2_11_0="ATN064041SENSORE\r";
const char *str2_11_1="ATN064051ESCLUSO\r";
const char *str2_11_2="ATN064071ESCLUDERE?\r";
const char *str2_11_3="ESCLUDI SENS.";
const char *str2_11_4="";//ATN064041SENSORE\r";
const char *str2_11_5="ATN064051INCLUSO\r";
const char *str2_11_6="ATN064071INCLUDERE?\r";
const char *str2_11_7="INCLUDI SENS.";
const char *str2_11_8="ATN064041ACCETTATO\r";
const char *str2_11_9=" All";
const char *str2_11_10=" Gua";
const char *str2_11_11=" Man";
const char *str2_11_12="ACC. ALL. SENS.";
const char *str2_11_13="ATN064041ACCETTAZIONE\r";
const char *str2_11_14="ATN064051TOTALE ESEGUITA\r";
const char *str2_11_15="ACCETTA TOTALE";
const char *str2_11_16="ATN064041EFFETTUARE\r";
const char *str2_11_17="ATN064051ACCETTAZIONE\r";
const char *str2_11_18="ATN064061TOTALE?\r";
const char *str2_11_19="ATN064041%s\r";
const char *str2_11_20="ATN064051%2$s\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_sicurezza.c */
const char *str2_12_0="ATN064041PASSWORD\r";
const char *str2_12_1="ATN064051ERRATA\r";
const char *str2_12_2="";//ATN0504PASSWORD\r";
const char *str2_12_3="ATN064051MODIFICATA\r";
const char *str2_12_4="ATN064041CREATA NUOVA\r";
const char *str2_12_5="ATN064051UTENZA\r";
const char *str2_12_6="ATN064051MAX 6 CIFRE\r";
const char *str2_12_7="ATN064071CONF. PASSWORD\r";
//const char *str2_12_8="MODIFICA";
//const char *str2_12_9="ATN064021PASSWORD\r";
const char *str2_12_8="";//MODIFICA";
const char *str2_12_9="ATN064011MODIFICA PASSWORD\r";
const char *str2_12_10="ATN064041UTENZA\r";
const char *str2_12_11="ATN064051GENERICA\r";
const char *str2_12_12="CREA UTENZA";
const char *str2_12_13="ATN064041DIGIT. PASSWORD\r";
const char *str2_12_14="ATN064041LIVELLO UTENZA\r";
const char *str2_12_15="ATN064051NON CORRETTO\r";
const char *str2_12_16="ATN064031CREARE PRIMA\r";
const char *str2_12_17="ATN064041UNA UTENZA DI\r";
const char *str2_12_18="ATN064051LIVELLO %d\r";
const char *str2_12_19="ATN064071PROFILO UTENZA\r";
const char *str2_12_20="";//CREA UTENZA";
const char *str2_12_21="ATN064041ID UTENZA\r";
const char *str2_12_22="";//ATN0604UTENZA\r";
const char *str2_12_23="ATN064051INESISTENTE\r";
const char *str2_12_24="";//ATN0604UTENZA\r";
const char *str2_12_25="ATN064051CORRENTE\r";
const char *str2_12_26="";//ATN0604UTENZA\r";
const char *str2_12_27="";//ATN0305INESISTENTE\r";
const char *str2_12_28="ATN064041UTENTE\r";
const char *str2_12_29="";//ATN0505CORRENTE\r";
const char *str2_12_30="";//ATN0604UTENZA\r";
const char *str2_12_31="ATN064051CANCELLATA\r";
const char *str2_12_32="ATN064041CANCELLARE\r";
const char *str2_12_33="ATN064051UTENZA?\r";
const char *str2_12_34="(Profilo %d)";
const char *str2_12_35="CANCELLA UTENZA";
const char *str2_12_36="ATN040080______\r";
const char *str2_12_37="ATG04008\r";
const char *str2_12_38="ATN040050______\r";
const char *str2_12_39="ATG04005\r";
const char *str2_12_40="ATN06208_\r";
const char *str2_12_41="ATG06208\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_sms.c */
const char *str2_13_0="ATN064041SMS INVIATO\r";
const char *str2_13_1="ATN064041DIGITARE NUMERO\r";
const char *str2_13_2="INVIA SMS";
const char *str2_13_3="ATN064031DIGITARE TESTO\r";
const char *str2_13_4="ATN064051MESSAGGIO\r";
const char *str2_13_5="ATN064061CANCELLATO\r";
const char *str2_13_6="ATN064051CANCELLARE\r";
const char *str2_13_7="ATN064061MESSAGGIO?\r";
const char *str2_13_9="ATN000080CANCELLA\r";
const char *str2_13_10="ATN064041GSM impegnato su\r";
const char *str2_13_11="ATN064051altro terminale\r";
const char *str2_13_12="LEGGI SMS";
const char *str2_13_13="ATN064041Caricamento\r";
const char *str2_13_14="ATN064051messaggi\r";
const char *str2_13_15="CREDITO";
const char *str2_13_16="ATN064041Inoltrata\r";
const char *str2_13_17="ATN064051richiesta\r";
const char *str2_13_18="";
const char *str2_13_19="ATN064071Errore SIM\r";
const char *str2_13_20="ATG00006\r";
const char *str2_13_21="ATG00004\r";
const char *str2_13_22="ATN000%02d0"; //%.22s\t\r";
const char *str2_13_23="ATN000030%s\r";
const char *str2_13_24="ATN000040%s\r";
const char *str2_13_25="ATN000050%s:\r";
const char *str2_13_26="ATN000060%s\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/console.c */
const char *str2_14_0="DISATTIVA";
const char *str2_14_1="ATTIVA";
const char *str2_14_2="ATTIVA con ALL.";
const char *str2_14_3="ATTIVA con ESCL.";
const char *str2_14_4="SENS. in ALLARME";
const char *str2_14_5="SENS. da ACCETT.";
const char *str2_14_6="SENS. ESCLUSI";
const char *str2_14_7="SENS. GUASTI";
const char *str2_14_8="SENS. MANOMESSI";
const char *str2_14_9="ZONE ATTIVATE";
const char *str2_14_10="ZONE DISATTIVATE";
const char *str2_14_11="ATT. in stato ON";
const char *str2_14_12="ATT. ESCLUSI";
const char *str2_14_13="NUMERI CHIAMATI";
const char *str2_14_14="FESTIVI";
const char *str2_14_15="INVIA";
const char *str2_14_16="PERIFERICHE";
const char *str2_14_17="ZONE/SENSORI";
const char *str2_14_18="INCLUDI";
const char *str2_14_19="ESCLUDI";
const char *str2_14_20="ACCETTA SENSORE";
const char *str2_14_21="ACCETTA TOTALE";
const char *str2_14_22="INCLUDI";
const char *str2_14_23="ESCLUDI";
const char *str2_14_24="OFF ATTUATORE";
const char *str2_14_25="ON ATTUATORE";
const char *str2_14_26="INVIA";
const char *str2_14_27="LEGGI";
const char *str2_14_28="CREDITO";
const char *str2_14_29="FASCE ORARIE";
const char *str2_14_30="FESTIVI";
const char *str2_14_31="DATA E ORA";
const char *str2_14_32="ASS. ZONE/SENS.";
const char *str2_14_33="ASS. ZS/ZI";
const char *str2_14_34="ZONE RITARDATE";
const char *str2_14_35="TIMEOUT LOGIN";
const char *str2_14_36="CODICI DIRETTI";
const char *str2_14_37="ZONE";
const char *str2_14_38="SENSORI";
const char *str2_14_39="ATTUATORI";
const char *str2_14_40="TITOLARI NUMERI";
const char *str2_14_41="TELECOMANDI";
const char *str2_14_42="CODICI";
const char *str2_14_43="ABILITA";
const char *str2_14_44="DISABILITA";
const char *str2_14_45="CARICA NUMERO";
const char *str2_14_46="CANCELLA NUMERO";
const char *str2_14_47="ABILITA NUMERO";
const char *str2_14_48="DISABILITA NUM.";
const char *str2_14_49="MESSAGGI AUDIO";
const char *str2_14_50="MODIFICA PASSWD";
const char *str2_14_51="CREA UTENZA";
const char *str2_14_52="CANCELLA UTENZA";
const char *str2_14_53="SEGRETI";
const char *str2_14_54="EVENTI";
const char *str2_14_55="REAL TIME";
const char *str2_14_56="CONSOLIDATO";
const char *str2_14_57="CANCELLA STORICO";
const char *str2_14_58="PROFILI ";
const char *str2_14_59="AREE";
const char *str2_14_60="FESTIVITA'";
const char *str2_14_61="FASCE SETTIM.";
const char *str2_14_62="TERMINALI";
const char *str2_14_63="ZONE";
const char *str2_14_64="SENSORI";
const char *str2_14_65="ATTUATORI";
const char *str2_14_66="TITOLARI NUMERI";
const char *str2_14_67="TELECOMANDI";
const char *str2_14_68="CODICI";
const char *str2_14_69="VISUALIZZA";
const char *str2_14_70="MODIFICA";
const char *str2_14_71="VISUALIZZA";
const char *str2_14_72="MODIFICA";
const char *str2_14_73="VISUALIZZA";
const char *str2_14_74="MODIFICA";
const char *str2_14_75="FASCE SETTIMAN.";
const char *str2_14_76="FESTIVITA'";
const char *str2_14_77="DATA E ORA";
const char *str2_14_78="ASS. ZONE/SENS.";
const char *str2_14_79="ASS. ZS/ZI";
const char *str2_14_80="TIMEOUT LOGIN";
const char *str2_14_81="CODICI DIRETTI";
const char *str2_14_82="ZONE";
const char *str2_14_83="LISTE";
const char *str2_14_84="TELECOMANDI";
const char *str2_14_85="SENSORI";
const char *str2_14_86="ATTUATORI";
const char *str2_14_87="IMPOSTAZIONI";
const char *str2_14_88="SMS";
const char *str2_14_89="PERSONALIZZA";
const char *str2_14_90="BADGE";
const char *str2_14_91="CHIAVI";
const char *str2_14_92="MODEM";
const char *str2_14_93="SICUREZZA";
const char *str2_14_94="INOLTRA";
const char *str2_14_95="STORICO";
const char *str2_14_96="LOGOUT";
const char *str2_14_97="ZONE";
const char *str2_14_98="LISTE";
const char *str2_14_99="TELECOMANDI";
const char *str2_14_100="SENSORI";
const char *str2_14_101="ATTUATORI";
const char *str2_14_102="IMPOSTAZIONI";
const char *str2_14_103="SMS";
const char *str2_14_104="PERSONALIZZA";
const char *str2_14_105="MODEM";
const char *str2_14_106="SICUREZZA";
const char *str2_14_107="INOLTRA";
const char *str2_14_108="STORICO";
const char *str2_14_109="PASS";
const char *str2_14_110="PROFILI";
const char *str2_14_111="TERMINALI";
const char *str2_14_112="LOGOUT";
const char *str2_14_113="Gen";
const char *str2_14_114="Feb";
const char *str2_14_115="Mar";
const char *str2_14_116="Apr";
const char *str2_14_117="Mag";
const char *str2_14_118="Giu";
const char *str2_14_119="Lug";
const char *str2_14_120="Ago";
const char *str2_14_121="Set";
const char *str2_14_122="Ott";
const char *str2_14_123="Nov";
const char *str2_14_124="Dic";
const char *str2_14_125="ATN064%02d1(Vuoto)\r";
const char *str2_14_126="ATN000010Zona n.%d\r";
const char *str2_14_127="ATN000010Zona n.%d\r";
const char *str2_14_128="ATN064041CODICE\r";
const char *str2_14_129="ATN064051NON VALIDO\r";
const char *str2_14_130=""; //ATN064011INVIO CODICE\r";
const char *str2_14_131=""; //ATN040040CODICE\r";
const char *str2_14_132="ATN060030CODICE\r";
const char *str2_14_133="    %d- Login utente %s";
const char *str2_14_134="ATN064051ERRATO\r";
const char *str2_14_135="ATN064041DIGITARE\r";
const char *str2_14_136="ATN064051PASSWORD\r";
const char *str2_14_137="ATN064011LOGIN\r";
const char *str2_14_138="ATN064041DIGITARE ID\r";
const char *str2_14_139="Console: modificato baudrate";
const char *str2_14_140="ATN064041Sistema TEBE\r";
const char *str2_14_141="ATN064051non attivo\r";
const char *str2_14_142="INDIRIZZO IP    ";

const char *str2_14_143="ATN000%02d0  %s\t\r";
const char *str2_14_144="ATN008010%s\r";
const char *str2_14_145="ATN000%02d0    %s\t\r";
const char *str2_14_146="ATN063011%2$s\r";
const char *str2_14_147="ATU0000212801\r";
const char *str2_14_148="ATN048040LOGOUT?\r";
const char *str2_14_149="ATN0010%d0%s\r";
const char *str2_14_150="ATN000040%s\r";
const char *str2_14_151="ATN000050%s\r";
const char *str2_14_152="ATN0110%d0 %c \r";
const char *str2_14_153="ATN0320%d0|\r";
const char *str2_14_154="ATN064050____\r";
const char *str2_14_155="ATG06405\r";
const char *str2_14_156="ATN064041%02d/%02d/%02d\r";
const char *str2_14_157="ATN064051%02d:%02d:%02d\r";
const char *str2_14_158="ATN064041LOGIN\r";
const char *str2_14_159="ATN040070______\r";
const char *str2_14_160="ATG04007\r";
const char *str2_14_161="ATN040060______\r";
const char *str2_14_162="ATG04006\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_storico.c */
const char *str2_15_50="Allarme         sensore $0";
const char *str2_15_51="Ripristino      sensore $0";
const char *str2_15_52="Guasto          perif. $4";
const char *str2_15_53="Fine guasto     perif. $4";
const char *str2_15_54="Manomissione    sensore $0";
const char *str2_15_55="Fine manomiss.  sensore $0";
const char *str2_15_56="Manomissione    contenitore     perif. $4";
const char *str2_15_57="Fine manomiss.  contenitore     perif. $4";
const char *str2_15_58="Attivata        zona $2";
const char *str2_15_59="Disattivata     zona $2";
const char *str2_15_60="Att. impedita   zona $2";
const char *str2_15_61="In servizio     sensore $0";
const char *str2_15_62="Fuori servizio  sensore $0";
const char *str2_15_63="In servizio     attuatore $1";
const char *str2_15_64="Fuori servizio  attuatore $1";
const char *str2_15_65="Mod. Master $3:$nricevuto$ncodice errato";
const char *str2_15_66="Mod. Master $3:$ntipo $3";
const char *str2_15_67="Perif. incongr. ind. $4$ntipo $3";
const char *str2_15_68="Perif. manomessaind. $4";
const char *str2_15_69="Ripristino perifind. $4";
const char *str2_15_70="Sospesa attivitàlinea $3";
const char *str2_15_71="Chiave falsa    perif. $4";
const char *str2_15_72="Sent. $4$nattivata";
const char *str2_15_73="Sent. $4$ndisattivata";
const char *str2_15_74="Sent. $4$ndisattivata$ntimeout";
const char *str2_15_75="Mod. Master $3:$nErrore Tx";
const char *str2_15_76="Mod. Master $3:$nErrore Rx";
const char *str2_15_77="Errore ric.$nmessaggio host";
const char *str2_15_78="Segnalazione$nevento n.$6";
const char *str2_15_79="Perif. presenti$n$4 >$n$5$5$5$5$5$5$5$5";
const char *str2_15_80="Stato zone   $8$a$a$a$a$a$a$a$a";
const char *str2_15_81="Stato sens. $6$a$a$a$a$a$a$a$a";
const char *str2_15_82="Stato att.  $6$a$a$a$a$a$a$a$a";
const char *str2_15_83="Guasto          sensore $0";
const char *str2_15_84="Variazione ora  $9";
const char *str2_15_85="Codici$ncontrollo$n$x-$x-$x";
//const char *str2_15_87="Stato QRA I=$3 QRA II=$3. Att.ronda $3";
const char *str2_15_88="Accett. allarme sensore $0";
const char *str2_15_89="No punzonatura  stazione $4";
const char *str2_15_90="On telecomando  $7";
const char *str2_15_91="Param. taratura perif. $4$nSens.$3 Dur.$3";
const char *str2_15_92="Stato Prova = $3";
const char *str2_15_93="Test attivo     gruppo $8";
const char *str2_15_94="Risposta SCS";
const char *str2_15_95="Fine guasto     sensore $0";
const char *str2_15_96="Valore $6$nper segreto $3";
const char *str2_15_97="Perif. previste$n$4 >$n$5$5$5$5$5$5$5$5";
const char *str2_15_98="Sens. in allarmesensore $0$nZ=$2";
const char *str2_15_99="Variata fascia$noraria $8:$n$f ore $9";
//const char *str2_15_101="Invio diretto dati t$3 a host computer";
const char *str2_15_102="";
const char *str2_15_103="Fine invio dati";
//const char *str2_15_105="Cnf.ronda $3- $3,$3,$3,$3,$3,$3,$3,$3,$3,$3,$3,$3,$3,$3,$3,$3";
//const char *str2_15_107="Ore ronda $3- $9,$9,$9,$9,$9,$9";
const char *str2_15_108="Lista festivi $3:$3$3$3$3$3$3$3$3$3$3$3$3$3$3$3$3";
const char *str2_15_109="Fasce giorn. $8$n$9 $9$n$9 $9";
const char *str2_15_110="Attivato        attuatore $1";
const char *str2_15_111="Transito id $6lettore $4";
const char *str2_15_112="Entrato id. $6lettore $4";
const char *str2_15_113="Uscito  id. $6lettore $4";
const char *str2_15_114="Codice valido   $8";
const char *str2_15_115="Chiave valida   $8";
const char *str2_15_116="Operatore $8";
const char *str2_15_117="Stazione $6   punzonata";
const char *str2_15_118="Spegnimento     n. $6";
const char *str2_15_119="Reset fumo      n. $6";
const char *str2_15_120="Livello $8     abilitato";
const char *str2_15_121="Variato segreto";
const char *str2_15_122="Conness. modem  tipo $3";
const char *str2_15_123="Numero telef.  primario -";
const char *str2_15_124="Numero telef.  secondario -";
const char *str2_15_125="Esaminato Badge $4:$6";
const char *str2_15_126="Ev. sens. $6$n$e$n$E";
const char *str2_15_127="Stato MU $6$n$a$a$a$a$a$a$a$a";
const char *str2_15_128="Sens. $6 Zone:$8 $8 $8 $8 $8 $8 $8 $8";
//const char *str2_15_130="Descr.Zona $2";
//const char *str2_15_132="Descr.Dispositivo $6";
const char *str2_15_133="Fra $3 giorni$nvariato a $3";
const char *str2_15_134="Val. anal. $6$n$X$X$X$X$X$X$X$X";
const char *str2_15_135="Letto badge     $3-xxxxxxxxxx";
const char *str2_15_136="GSM $3:$n$s";
const char *str2_15_137="TMD $3:$n$s";
const char *str2_15_138="Tebe ACK        m:$3 t:$3 id:$i";
const char *str2_15_139="Lettura dati    tessera $i";
const char *str2_15_140="Lettura dati    profilo $D";
const char *str2_15_141="Lettura dati    terminale $d";
const char *str2_15_142="Lettura dati    area $D: $i";
const char *str2_15_143="Lettura dati    fascia oraria $d";
const char *str2_15_144="Lettura dati    festivo $d";
const char *str2_15_145="Id $i trans. Term $d Area $D";
const char *str2_15_146="Id $i timbr. Term $d Fe $3";
const char *str2_15_147="Id $i trans. Term $d Area $Dcodice $i";
const char *str2_15_148="Id $i timbr. Term $d Fe $3$ncodice $i";
const char *str2_15_149="Id $i F.Area Term $d";
const char *str2_15_150="Id $i N.Abil Term $d";
const char *str2_15_151="Id $i No Tr. F.O. su Term $d";
const char *str2_15_152="Id $i No Tr. Fest. su Term $d";
const char *str2_15_153="Id $i Pf $D  N.Abil Term. $d";
const char *str2_15_154="Id $i abilitagrado resp.$nda Term $d";
const char *str2_15_155="Ident. $i$nSegnalata$nCoercizione";
const char *str2_15_156="Id Assente$nNo transito$nda Term. $d";
const char *str2_15_157="";
const char *str2_15_158="";
const char *str2_15_159="Forzatura porta da Term. $d";
const char *str2_15_160="Apert. prolung. porta da$nTerm. $d";
const char *str2_15_161="Fine forz./ap.  prolungata portada Term. $d";
const char *str2_15_162="Allarme IngressoAux su Term. $d";
const char *str2_15_163="Fine allarme    Ingresso Aux    su Term. $d";
const char *str2_15_164="Bassa tensione  su Term. $d";
const char *str2_15_165="Fine bassa tens.su Term. $d";
const char *str2_15_166="";
const char *str2_15_167="";
const char *str2_15_168="";
const char *str2_15_169="Ident. $i$nvariato PIN";
const char *str2_15_170="Ident. $i$ndisabilitato";
const char *str2_15_171="Ident. $i$nabilitato";
const char *str2_15_172="Ident. $i$ninserito da$nTerm. $d";
const char *str2_15_173="Ident. $i$ninserito su$nCentrale";
const char *str2_15_174="Ident. $i$nforzato in$nArea $D";
const char *str2_15_175="Ident. $i$nassociato a$nProfilo $D";
const char *str2_15_176="Ident. $i$nassociato a$nF.O. Sett. $d";
const char *str2_15_177="Ident. $i$nvariata assoc.  a Terminali";
const char *str2_15_178="Ident. $i$nvariata assoc.  a Festivita'";
const char *str2_15_179="Ident. $i$nvariato Stato   $S1";
const char *str2_15_180="Ident. $i$nvariato Cont.";
const char *str2_15_181="Profilo $D$nassociato a$nF.O. Sett. $d";
const char *str2_15_182="Profilo $D$nvariata assoc.  a Terminali";
const char *str2_15_183="Profilo $D$nvariata assoc.  a Festivita'";
const char *str2_15_184="Profilo $D$nvariato Stato   $S2";
const char *str2_15_185="Term. $d config.$F";
const char *str2_15_186="Term. $d config.Fe1>area $D$nFe2>area $D";
const char *str2_15_187="Term. $d assoc. a F.O. Sett. $d";
const char *str2_15_188="Term. $d variataassociazione a$nFestivita'";
const char *str2_15_189="Term. $d$nvariato DIN/Cont";
const char *str2_15_190="Term. $d$nTempi:  TAP=$D TAZ=$D TPBK=$d";
const char *str2_15_191="Term. $d$nvariato Stato   $S3";
const char *str2_15_192="Variata F.O.$nSettimanale $d";
const char *str2_15_193="Variata$nFestivita' $d";
const char *str2_15_194="";
const char *str2_15_195="";
const char *str2_15_196="";
const char *str2_15_197="";
const char *str2_15_198="Reset Centrale  $i";
const char *str2_15_199="Ident. $i$nTerm. $d$nRichiesta $i";
const char *str2_15_200="Presenze$nArea $D: $i";
const char *str2_15_201="Inizio$ntest attivo$ngruppo $3";
const char *str2_15_202="Fine$ntest attivo$ngruppo $3";
const char *str2_15_203="allarme";
const char *str2_15_204="manomissione";
const char *str2_15_205="guasto";
const char *str2_15_206="iniziato";
const char *str2_15_208="accettato";
const char *str2_15_209="non accettato";
const char *str2_15_210="fine";
const char *str2_15_211="inizio";
const char *str2_15_212="B+S";
const char *str2_15_213="B  ";
const char *str2_15_214="S  ";
const char *str2_15_215="   ";
const char *str2_15_216="Fin=%s Fout=%s";
const char *str2_15_217="Coer ";
const char *str2_15_218="PBK ";
const char *str2_15_219="SV ";
const char *str2_15_220="Canc";
const char *str2_15_221="Coerc:%c  APBK:%c";
const char *str2_15_222="APBK:%c BlkTas:%c";
const char *str2_15_223="ATN064051CANCELLARE\r";
const char *str2_15_224="ATN064061STORICO?\r";
const char *str2_15_225="";//ATN0405CANCELLARE\r";
const char *str2_15_226="ATN064061EVENTO?\r";
const char *str2_15_227="REALTIME";
const char *str2_15_228="";//ATN0405CANCELLARE\r";
const char *str2_15_229="";//ATN0506STORICO?\r";
const char *str2_15_230="";//ATN0405CANCELLARE\r";
const char *str2_15_231="";//ATN0606EVENTO?\r";
const char *str2_15_232="ATN000030Progressivo %04d\r";
const char *str2_15_233="ATN064041(Vuoto)\r";
const char *str2_15_234="CONSOLIDATO";
const char *str2_15_235="ATN064041STORICO\r";
const char *str2_15_236="ATN064051CANCELLATO\r";
const char *str2_15_237="CANCELLA STORICO";
const char *str2_15_238="";//ATN0405CANCELLARE\r";
const char *str2_15_239="";//ATN0506STORICO?\r";
const char *str2_15_240="Evento$nsconosciuto";
const char *str2_15_241="Allineamento$ncentrale";
const char *str2_15_242="ATN000040";
const char *str2_15_243="ATN000%02d0%.16s\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_telecomandi.c */
const char *str2_16_0="ATN064041TELECOMANDO\r";
const char *str2_16_1="ATN064051INVIATO\r";
const char *str2_16_2="ATN064061INVIARE?\r";
const char *str2_16_3="TELECOMANDI";
const char *str2_16_4="PERIFERICHE";
const char *str2_16_5="ATN064041Lista\r";
const char *str2_16_6="ATN064051periferiche\r";
const char *str2_16_7="ATN064061in storico\r";
const char *str2_16_8="ZONE/SENSORI";
const char *str2_16_9="ATN064041Lista\r";
const char *str2_16_10="ATN064051sensore/zona\r";
const char *str2_16_11="ATN064061in storico\r";
const char *str2_16_12="ATN064041%s\r";
const char *str2_16_13="ATN064051%2$s\r";

/* /home/gianx/lavoro/saet/axis3/devboard_lx/apps/saet/console/con_zone.c */
const char *str2_17_0="ATN064041ZONA\r";
const char *str2_17_1="ATN064051DISATTIVATA\r";
const char *str2_17_2="ATN064041ZONA %s\r";
const char *str2_17_3="ATN064061DISATTIVARE?\r";
const char *str2_17_4="DISATTIVA ZONA";
const char *str2_17_5="ZONA %s";
const char *str2_17_6="ATN064041ZONA\r";
const char *str2_17_7="ATN064051ATTIVATA\r";
const char *str2_17_8="ATN064041ZONA %s\r";
const char *str2_17_9="ATN064061ATTIVARE?\r";
const char *str2_17_10="ATTIVA ZONA";
const char *str2_17_11="ZONA %s";
const char *str2_17_12="";//ATN0704ZONA\r";
const char *str2_17_13="";//ATN0505ATTIVATA\r";
const char *str2_17_14="";//ATN0404ZONA %s\r";
const char *str2_17_15="";//ATN0406ATTIVARE?\r";
const char *str2_17_16="";//ATTIVA ZONA";
const char *str2_17_17="";//ZONA %s";
const char *str2_17_19="";//ATN0704ZONA\r";
const char *str2_17_20="";//ATN0505ATTIVATA\r";
const char *str2_17_21="";//ATN0404ZONA %s\r";
const char *str2_17_22="";//ATN0406ATTIVARE?\r";
const char *str2_17_23="";//ATTIVA ZONA";
const char *str2_17_24="";//ZONA %s";

const char **console_str2[CONSOLE_STRINGS] = {
  &str2_0_0,
  &str2_0_1,
  &str2_0_2,
  &str2_0_3,
  &str2_0_4,
  &str2_0_5,
  &str2_0_6,
  &str2_0_7,
  &str2_0_8,
  &str2_0_9,
  &str2_0_10,
  &str2_0_11,
  &str2_0_12,
  &str2_0_13,
  &str2_1_0,
  &str2_1_1,
  &str2_1_2,
  &str2_1_3,
  &str2_1_4,
  &str2_1_5,
  &str2_1_6,
  &str2_1_7,
  &str2_1_8,
  &str2_1_9,
  &str2_1_10,
  &str2_2_0,
  &str2_2_1,
  &str2_2_2,
  &str2_2_3,
  &str2_2_4,
  &str2_2_5,
  &str2_2_6,
  &str2_2_7,
  &str2_2_8,
  &str2_2_9,
  &str2_2_10,
  &str2_2_11,
  &str2_2_12,
  &str2_2_13,
  &str2_2_14,
  &str2_2_15,
  &str2_2_16,
  &str2_2_17,
  &str2_2_18,
  &str2_2_19,
  &str2_2_20,
  &str2_2_23,
  &str2_2_24,
  &str2_2_25,
  &str2_2_26,
  &str2_2_27,
  &str2_2_28,
  &str2_2_29,
  &str2_2_30,
  &str2_2_31,
  &str2_2_32,
  &str2_2_33,
  &str2_2_34,
  &str2_2_35,
  &str2_2_36,
  &str2_2_37,
  &str2_2_38,
  &str2_2_39,
  &str2_2_40,
  &str2_2_41,
  &str2_2_42,
  &str2_2_43,
  &str2_2_44,
  &str2_2_45,
  &str2_2_46,
  &str2_2_47,
  &str2_2_48,
  &str2_2_49,
  &str2_2_50,
  &str2_2_51,
  &str2_2_52,
  &str2_2_53,
  &str2_3_0,
  &str2_3_1,
  &str2_3_2,
  &str2_3_3,
  &str2_3_4,
  &str2_3_5,
  &str2_3_6,
  &str2_3_7,
  &str2_3_8,
  &str2_3_9,
  &str2_3_10,
  &str2_3_11,
  &str2_3_12,
  &str2_3_13,
  &str2_3_14,
  &str2_3_15,
  &str2_3_16,
  &str2_3_17,
  &str2_3_18,
  &str2_3_19,
  &str2_4_0,
  &str2_4_1,
  &str2_4_2,
  &str2_4_3,
  &str2_4_4,
  &str2_4_5,
  &str2_4_6,
  &str2_4_7,
  &str2_4_8,
  &str2_4_9,
  &str2_4_10,
  &str2_4_11,
  &str2_4_12,
  &str2_4_13,
  &str2_4_14,
  &str2_4_15,
  &str2_4_16,
  &str2_4_17,
  &str2_4_18,
  &str2_4_19,
  &str2_4_20,
  &str2_4_21,
  &str2_4_22,
  &str2_4_23,
  &str2_4_24,
  &str2_4_25,
  &str2_4_33,
  &str2_4_41,
  &str2_4_42,
  &str2_4_43,
  &str2_4_44,
  &str2_4_45,
  &str2_4_46,
  &str2_4_47,
  &str2_4_48,
  &str2_4_49,
  &str2_4_50,
  &str2_4_51,
  &str2_4_52,
  &str2_4_53,
  &str2_4_54,
  &str2_4_55,
  &str2_4_56,
  &str2_4_57,
  &str2_5_0,
  &str2_5_1,
  &str2_5_2,
  &str2_5_3,
  &str2_5_4,
  &str2_5_5,
  &str2_5_6,
  &str2_5_7,
  &str2_5_8,
  &str2_5_9,
  &str2_5_10,
  &str2_5_11,
  &str2_5_12,
  &str2_5_13,
  &str2_5_14,
  &str2_5_15,
  &str2_5_16,
  &str2_5_17,
  &str2_5_18,
  &str2_5_19,
  &str2_5_20,
  &str2_5_21,
  &str2_5_22,
  &str2_5_23,
  &str2_5_24,
  &str2_5_25,
  &str2_5_26,
  &str2_5_27,
  &str2_5_28,
  &str2_5_29,
  &str2_5_30,
  &str2_5_31,
  &str2_5_32,
  &str2_5_33,
  &str2_5_34,
  &str2_5_35,
  &str2_5_36,
  &str2_5_37,
  &str2_5_38,
  &str2_5_39,
  &str2_5_40,
  &str2_5_41,
  &str2_5_42,
  &str2_5_43,
  &str2_5_44,
  &str2_5_45,
  &str2_5_46,
  &str2_5_47,
  &str2_5_48,
  &str2_5_49,
  &str2_5_50,
  &str2_5_51,
  &str2_5_52,
  &str2_5_53,
  &str2_5_54,
  &str2_5_55,
  &str2_5_56,
  &str2_5_57,
  &str2_5_58,
  &str2_5_59,
  &str2_5_60,
  &str2_5_61,
  &str2_5_62,
  &str2_5_63,
  &str2_5_64,
  &str2_5_65,
  &str2_5_66,
  &str2_5_67,
  &str2_5_68,
  &str2_5_69,
  &str2_5_70,
  &str2_5_71,
  &str2_5_72,
  &str2_5_73,
  &str2_5_74,
  &str2_5_75,
  &str2_5_76,
  &str2_5_77,
  &str2_5_78,
  &str2_5_79,
  &str2_5_80,
  &str2_5_81,
  &str2_5_82,
  &str2_5_83,
  &str2_5_84,
  &str2_5_85,
  &str2_5_86,
  &str2_5_87,
  &str2_5_88,
  &str2_5_89,
  &str2_5_90,
  &str2_5_91,
  &str2_5_92,
  &str2_5_93,
  &str2_5_94,
  &str2_5_95,
  &str2_5_96,
  &str2_5_97,
  &str2_5_98,
  &str2_5_99,
  &str2_5_100,
  &str2_5_101,
  &str2_5_102,
  &str2_5_103,
  &str2_5_104,
  &str2_5_105,
  &str2_5_106,
  &str2_5_107,
  &str2_5_108,
  &str2_5_109,
  &str2_5_110,
  &str2_5_111,
  &str2_5_112,
  &str2_5_113,
  &str2_5_114,
  &str2_5_115,
  &str2_5_116,
  &str2_5_117,
  &str2_5_118,
  &str2_5_119,
  &str2_5_120,
  &str2_5_121,
  &str2_5_122,
  &str2_5_123,
  &str2_5_124,
  &str2_5_125,
  &str2_5_126,
  &str2_5_127,
  &str2_5_128,
  &str2_5_129,
  &str2_5_130,
  &str2_5_131,
  &str2_5_132,
  &str2_5_133,
  &str2_5_134,
  &str2_5_135,
  &str2_5_136,
  &str2_5_137,
  &str2_5_138,
  &str2_5_139,
  &str2_5_140,
  &str2_5_141,
  &str2_6_0,
  &str2_6_1,
  &str2_6_2,
  &str2_6_3,
  &str2_6_4,
  &str2_6_5,
  &str2_6_6,
  &str2_6_7,
  &str2_6_8,
  &str2_6_9,
  &str2_6_10,
  &str2_6_11,
  &str2_6_12,
  &str2_6_13,
  &str2_6_14,
  &str2_6_15,
  &str2_6_16,
  &str2_6_17,
  &str2_6_18,
  &str2_6_19,
  &str2_6_20,
  &str2_6_21,
  &str2_6_22,
  &str2_6_23,
  &str2_6_24,
  &str2_6_25,
  &str2_6_26,
  &str2_6_27,
  &str2_6_28,
  &str2_6_29,
  &str2_6_30,
  &str2_6_31,
  &str2_6_32,
  &str2_6_33,
  &str2_6_34,
  &str2_6_35,
  &str2_6_36,
  &str2_6_37,
  &str2_6_38,
  &str2_6_39,
  &str2_6_40,
  &str2_6_41,
  &str2_6_42,
  &str2_6_43,
  &str2_6_44,
  &str2_6_45,
  &str2_6_46,
  &str2_6_47,
  &str2_6_48,
  &str2_6_49,
  &str2_6_50,
  &str2_6_51,
  &str2_6_52,
  &str2_6_53,
  &str2_6_54,
  &str2_6_55,
  &str2_6_56,
  &str2_6_57,
  &str2_6_58,
  &str2_6_59,
  &str2_6_60,
  &str2_6_61,
  &str2_6_62,
  &str2_6_63,
  &str2_6_64,
  &str2_6_65,
  &str2_6_66,
  &str2_6_67,
  &str2_6_68,
  &str2_6_69,
  &str2_6_70,
  &str2_6_71,
  &str2_7_0,
  &str2_7_1,
  &str2_7_2,
  &str2_7_3,
  &str2_7_4,
  &str2_7_5,
  &str2_7_6,
  &str2_7_7,
  &str2_7_8,
  &str2_7_9,
  &str2_7_10,
  &str2_7_11,
  &str2_7_12,
  &str2_7_13,
  &str2_7_14,
  &str2_7_15,
  &str2_7_16,
  &str2_7_17,
  &str2_7_18,
  &str2_7_19,
  &str2_7_20,
  &str2_7_21,
  &str2_7_22,
  &str2_7_23,
  &str2_7_24,
  &str2_7_25,
  &str2_7_26,
  &str2_7_27,
  &str2_7_29,
  &str2_7_30,
  &str2_7_31,
  &str2_7_33,
  &str2_7_34,
  &str2_7_35,
  &str2_7_36,
  &str2_7_37,
  &str2_7_38,
  &str2_7_39,
  &str2_7_40,
  &str2_7_41,
  &str2_7_42,
  &str2_7_43,
  &str2_7_44,
  &str2_7_45,
  &str2_7_46,
  &str2_7_47,
  &str2_7_48,
  &str2_7_49,
  &str2_7_50,
  &str2_7_51,
  &str2_7_52,
  &str2_7_53,
  &str2_7_54,
  &str2_7_55,
  &str2_7_56,
  &str2_7_57,
  &str2_7_58,
  &str2_7_59,
  &str2_7_60,
  &str2_7_61,
  &str2_7_62,
  &str2_7_63,
  &str2_7_64,
  &str2_7_65,
  &str2_7_66,
  &str2_7_67,
  &str2_7_68,
  &str2_7_69,
  &str2_7_70,
  &str2_7_71,
  &str2_7_72,
  &str2_7_73,
  &str2_7_74,
  &str2_7_75,
  &str2_7_76,
  &str2_7_77,
  &str2_7_81,
  &str2_7_82,
  &str2_7_83,
  &str2_7_84,
  &str2_7_85,
  &str2_7_86,
  &str2_7_87,
  &str2_7_88,
  &str2_7_89,
  &str2_7_90,
  &str2_7_91,
  &str2_7_92,
  &str2_7_93,
  &str2_7_94,
  &str2_7_95,
  &str2_7_96,
  &str2_7_97,
  &str2_7_98,
  &str2_7_99,
  &str2_7_100,
  &str2_7_101,
  &str2_7_102,
  &str2_7_103,
  &str2_7_104,
  &str2_7_105,
  &str2_7_106,
  &str2_7_107,
  &str2_7_108,
  &str2_7_109,
  &str2_7_110,
  &str2_7_111,
  &str2_7_112,
  &str2_7_113,
  &str2_7_114,
  &str2_7_115,
  &str2_7_116,
  &str2_7_117,
  &str2_7_118,
  &str2_7_119,
  &str2_7_120,
  &str2_7_121,
  &str2_7_122,
  &str2_7_123,
  &str2_7_124,
  &str2_7_125,
  &str2_7_126,
  &str2_7_127,
  &str2_7_128,
  &str2_7_129,
  &str2_7_130,
  &str2_7_131,
  &str2_7_132,
  &str2_7_133,
  &str2_7_134,
  &str2_7_136,
  &str2_8_0,
  &str2_8_1,
  &str2_8_2,
  &str2_8_3,
  &str2_8_4,
  &str2_8_5,
  &str2_8_6,
  &str2_8_7,
  &str2_8_8,
  &str2_8_9,
  &str2_8_10,
  &str2_8_11,
  &str2_8_12,
  &str2_8_14,
  &str2_8_15,
  &str2_8_16,
  &str2_8_17,
  &str2_8_18,
  &str2_8_19,
  &str2_8_20,
  &str2_8_21,
  &str2_8_22,
  &str2_8_23,
  &str2_8_24,
  &str2_8_25,
  &str2_8_26,
  &str2_8_27,
  &str2_8_28,
  &str2_8_29,
  &str2_8_30,
  &str2_8_31,
  &str2_8_32,
  &str2_8_33,
  &str2_8_34,
  &str2_8_35,
  &str2_8_36,
  &str2_8_38,
  &str2_8_39,
  &str2_8_40,
  &str2_8_41,
  &str2_8_42,
  &str2_8_43,
  &str2_8_44,
  &str2_8_45,
  &str2_8_46,
  &str2_8_47,
  &str2_8_48,
  &str2_8_49,
  &str2_9_0,
  &str2_9_1,
  &str2_9_2,
  &str2_9_3,
  &str2_9_4,
  &str2_9_5,
  &str2_9_6,
  &str2_9_7,
  &str2_9_8,
  &str2_9_9,
  &str2_9_10,
  &str2_9_11,
  &str2_9_12,
  &str2_9_13,
  &str2_9_14,
  &str2_9_15,
  &str2_9_16,
  &str2_9_17,
  &str2_9_18,
  &str2_9_19,
  &str2_9_20,
  &str2_9_21,
  &str2_9_28,
  &str2_9_29,
  &str2_9_30,
  &str2_9_31,
  &str2_9_32,
  &str2_9_33,
  &str2_9_34,
  &str2_9_35,
  &str2_9_36,
  &str2_9_37,
  &str2_9_38,
  &str2_9_39,
  &str2_9_40,
  &str2_9_41,
  &str2_9_42,
  &str2_9_43,
  &str2_9_44,
  &str2_9_45,
  &str2_9_46,
  &str2_9_47,
  &str2_10_0,
  &str2_10_1,
  &str2_10_2,
  &str2_10_3,
  &str2_10_4,
  &str2_10_5,
  &str2_10_6,
  &str2_10_7,
  &str2_10_9,
  &str2_10_10,
  &str2_10_11,
  &str2_10_12,
  &str2_10_13,
  &str2_10_14,
  &str2_10_15,
  &str2_11_0,
  &str2_11_1,
  &str2_11_2,
  &str2_11_3,
  &str2_11_4,
  &str2_11_5,
  &str2_11_6,
  &str2_11_7,
  &str2_11_8,
  &str2_11_9,
  &str2_11_10,
  &str2_11_11,
  &str2_11_12,
  &str2_11_13,
  &str2_11_14,
  &str2_11_15,
  &str2_11_16,
  &str2_11_17,
  &str2_11_18,
  &str2_12_0,
  &str2_12_1,
  &str2_12_2,
  &str2_12_3,
  &str2_12_4,
  &str2_12_5,
  &str2_12_6,
  &str2_12_7,
  &str2_12_8,
  &str2_12_9,
  &str2_12_10,
  &str2_12_11,
  &str2_12_12,
  &str2_12_13,
  &str2_12_14,
  &str2_12_15,
  &str2_12_16,
  &str2_12_17,
  &str2_12_18,
  &str2_12_19,
  &str2_12_20,
  &str2_12_21,
  &str2_12_22,
  &str2_12_23,
  &str2_12_24,
  &str2_12_25,
  &str2_12_26,
  &str2_12_27,
  &str2_12_28,
  &str2_12_29,
  &str2_12_30,
  &str2_12_31,
  &str2_12_32,
  &str2_12_33,
  &str2_12_34,
  &str2_12_35,
  &str2_13_0,
  &str2_13_1,
  &str2_13_2,
  &str2_13_3,
  &str2_13_4,
  &str2_13_5,
  &str2_13_6,
  &str2_13_7,
  &str2_13_9,
  &str2_13_10,
  &str2_13_11,
  &str2_13_12,
  &str2_13_13,
  &str2_13_14,
  &str2_13_15,
  &str2_13_16,
  &str2_13_17,
  &str2_13_18,
  &str2_13_19,
  &str2_14_0,
  &str2_14_1,
  &str2_14_2,
  &str2_14_3,
  &str2_14_4,
  &str2_14_5,
  &str2_14_6,
  &str2_14_7,
  &str2_14_8,
  &str2_14_9,
  &str2_14_10,
  &str2_14_11,
  &str2_14_12,
  &str2_14_13,
  &str2_14_14,
  &str2_14_15,
  &str2_14_16,
  &str2_14_17,
  &str2_14_18,
  &str2_14_19,
  &str2_14_20,
  &str2_14_21,
  &str2_14_22,
  &str2_14_23,
  &str2_14_24,
  &str2_14_25,
  &str2_14_26,
  &str2_14_27,
  &str2_14_28,
  &str2_14_29,
  &str2_14_30,
  &str2_14_31,
  &str2_14_32,
  &str2_14_33,
  &str2_14_34,
  &str2_14_35,
  &str2_14_36,
  &str2_14_37,
  &str2_14_38,
  &str2_14_39,
  &str2_14_40,
  &str2_14_41,
  &str2_14_42,
  &str2_14_43,
  &str2_14_44,
  &str2_14_45,
  &str2_14_46,
  &str2_14_47,
  &str2_14_48,
  &str2_14_49,
  &str2_14_50,
  &str2_14_51,
  &str2_14_52,
  &str2_14_53,
  &str2_14_54,
  &str2_14_55,
  &str2_14_56,
  &str2_14_57,
  &str2_14_58,
  &str2_14_59,
  &str2_14_60,
  &str2_14_61,
  &str2_14_62,
  &str2_14_63,
  &str2_14_64,
  &str2_14_65,
  &str2_14_66,
  &str2_14_67,
  &str2_14_68,
  &str2_14_69,
  &str2_14_70,
  &str2_14_71,
  &str2_14_72,
  &str2_14_73,
  &str2_14_74,
  &str2_14_75,
  &str2_14_76,
  &str2_14_77,
  &str2_14_78,
  &str2_14_79,
  &str2_14_80,
  &str2_14_81,
  &str2_14_82,
  &str2_14_83,
  &str2_14_84,
  &str2_14_85,
  &str2_14_86,
  &str2_14_87,
  &str2_14_88,
  &str2_14_89,
  &str2_14_90,
  &str2_14_91,
  &str2_14_92,
  &str2_14_93,
  &str2_14_94,
  &str2_14_95,
  &str2_14_96,
  &str2_14_97,
  &str2_14_98,
  &str2_14_99,
  &str2_14_100,
  &str2_14_101,
  &str2_14_102,
  &str2_14_103,
  &str2_14_104,
  &str2_14_105,
  &str2_14_106,
  &str2_14_107,
  &str2_14_108,
  &str2_14_109,
  &str2_14_110,
  &str2_14_111,
  &str2_14_112,
  &str2_14_113,
  &str2_14_114,
  &str2_14_115,
  &str2_14_116,
  &str2_14_117,
  &str2_14_118,
  &str2_14_119,
  &str2_14_120,
  &str2_14_121,
  &str2_14_122,
  &str2_14_123,
  &str2_14_124,
  &str2_14_125,
  &str2_14_126,
  &str2_14_127,
  &str2_14_128,
  &str2_14_129,
  &str2_14_130,
  &str2_14_131,
  &str2_14_132,
  &str2_14_133,
  &str2_14_134,
  &str2_14_135,
  &str2_14_136,
  &str2_14_137,
  &str2_14_138,
  &str2_14_139,
  &str2_14_140,
  &str2_14_141,
  &str2_15_50,
  &str2_15_51,
  &str2_15_52,
  &str2_15_53,
  &str2_15_54,
  &str2_15_55,
  &str2_15_56,
  &str2_15_57,
  &str2_15_58,
  &str2_15_59,
  &str2_15_60,
  &str2_15_61,
  &str2_15_62,
  &str2_15_63,
  &str2_15_64,
  &str2_15_65,
  &str2_15_66,
  &str2_15_67,
  &str2_15_68,
  &str2_15_69,
  &str2_15_70,
  &str2_15_71,
  &str2_15_72,
  &str2_15_73,
  &str2_15_74,
  &str2_15_75,
  &str2_15_76,
  &str2_15_77,
  &str2_15_78,
  &str2_15_79,
  &str2_15_80,
  &str2_15_81,
  &str2_15_82,
  &str2_15_83,
  &str2_15_84,
  &str2_15_85,
  &str2_15_88,
  &str2_15_89,
  &str2_15_90,
  &str2_15_91,
  &str2_15_92,
  &str2_15_93,
  &str2_15_94,
  &str2_15_95,
  &str2_15_96,
  &str2_15_97,
  &str2_15_98,
  &str2_15_99,
  &str2_15_102,
  &str2_15_103,
  &str2_15_108,
  &str2_15_109,
  &str2_15_110,
  &str2_15_111,
  &str2_15_112,
  &str2_15_113,
  &str2_15_114,
  &str2_15_115,
  &str2_15_116,
  &str2_15_117,
  &str2_15_118,
  &str2_15_119,
  &str2_15_120,
  &str2_15_121,
  &str2_15_122,
  &str2_15_123,
  &str2_15_124,
  &str2_15_125,
  &str2_15_126,
  &str2_15_127,
  &str2_15_128,
  &str2_15_133,
  &str2_15_134,
  &str2_15_135,
  &str2_15_136,
  &str2_15_137,
  &str2_15_138,
  &str2_15_139,
  &str2_15_140,
  &str2_15_141,
  &str2_15_142,
  &str2_15_143,
  &str2_15_144,
  &str2_15_145,
  &str2_15_146,
  &str2_15_147,
  &str2_15_148,
  &str2_15_149,
  &str2_15_150,
  &str2_15_151,
  &str2_15_152,
  &str2_15_153,
  &str2_15_154,
  &str2_15_155,
  &str2_15_156,
  &str2_15_157,
  &str2_15_158,
  &str2_15_159,
  &str2_15_160,
  &str2_15_161,
  &str2_15_162,
  &str2_15_163,
  &str2_15_164,
  &str2_15_165,
  &str2_15_166,
  &str2_15_167,
  &str2_15_168,
  &str2_15_169,
  &str2_15_170,
  &str2_15_171,
  &str2_15_172,
  &str2_15_173,
  &str2_15_174,
  &str2_15_175,
  &str2_15_176,
  &str2_15_177,
  &str2_15_178,
  &str2_15_179,
  &str2_15_180,
  &str2_15_181,
  &str2_15_182,
  &str2_15_183,
  &str2_15_184,
  &str2_15_185,
  &str2_15_186,
  &str2_15_187,
  &str2_15_188,
  &str2_15_189,
  &str2_15_190,
  &str2_15_191,
  &str2_15_192,
  &str2_15_193,
  &str2_15_194,
  &str2_15_195,
  &str2_15_196,
  &str2_15_197,
  &str2_15_198,
  &str2_15_199,
  &str2_15_200,
  &str2_15_201,
  &str2_15_202,
  &str2_15_203,
  &str2_15_204,
  &str2_15_205,
  &str2_15_206,
  &str2_15_208,
  &str2_15_209,
  &str2_15_210,
  &str2_15_211,
  &str2_15_212,
  &str2_15_213,
  &str2_15_214,
  &str2_15_215,
  &str2_15_216,
  &str2_15_217,
  &str2_15_218,
  &str2_15_219,
  &str2_15_220,
  &str2_15_221,
  &str2_15_222,
  &str2_15_223,
  &str2_15_224,
  &str2_15_225,
  &str2_15_226,
  &str2_15_227,
  &str2_15_228,
  &str2_15_229,
  &str2_15_230,
  &str2_15_231,
  &str2_15_232,
  &str2_15_233,
  &str2_15_234,
  &str2_15_235,
  &str2_15_236,
  &str2_15_237,
  &str2_15_238,
  &str2_15_239,
  &str2_16_0,
  &str2_16_1,
  &str2_16_2,
  &str2_16_3,
  &str2_16_4,
  &str2_16_5,
  &str2_16_6,
  &str2_16_7,
  &str2_16_8,
  &str2_16_9,
  &str2_16_10,
  &str2_16_11,
  &str2_17_0,
  &str2_17_1,
  &str2_17_2,
  &str2_17_3,
  &str2_17_4,
  &str2_17_5,
  &str2_17_6,
  &str2_17_7,
  &str2_17_8,
  &str2_17_9,
  &str2_17_10,
  &str2_17_11,
  &str2_17_12,
  &str2_17_13,
  &str2_17_14,
  &str2_17_15,
  &str2_17_16,
  &str2_17_17,
  &str2_17_19,
  &str2_17_20,
  &str2_17_21,
  &str2_17_22,
  &str2_17_23,
  &str2_17_24,
// 1020
  &str2_14_142,
  &str2_2_54,
  &str2_2_55,
  &str2_2_56,
  &str2_5_142,
  &str2_5_143,
  &str2_5_144,
  &str2_5_145,
  &str2_5_146,
  &str2_5_147,
  &str2_5_148,
  &str2_15_241,
  &str2_7_135,
  &str2_7_137,
  &str2_7_138,
  &str2_7_139,
  &str2_7_140,
  &str2_7_141,
  &str2_7_142,
// 1039
  &str2_15_242,
  &str2_15_243,
  &str2_16_12,
  &str2_16_13,
  &str2_0_14,
  &str2_0_15,
  &str2_11_19,
  &str2_11_20,
  &str2_2_57,
  &str2_2_58,
  &str2_2_59,
  &str2_2_60,
  &str2_2_61,
  &str2_2_62,
  &str2_2_63,
  &str2_2_64,
// 1055
  &str2_2_65,
  &str2_2_66,
  &str2_2_67,
  &str2_2_68,
  &str2_2_69,
  &str2_2_70,
  &str2_2_71,
  &str2_2_72,
  &str2_2_73,
  &str2_2_74,
  &str2_2_75,
  &str2_2_76,
  &str2_2_77,
  &str2_2_78,
  &str2_2_79,
  &str2_2_80,
  &str2_2_81,
  &str2_2_82,
  &str2_2_83,
  &str2_2_84,
  &str2_2_85,
  &str2_2_86,
  &str2_2_87,
  &str2_2_88,
  &str2_2_89,
// 1080
  &str2_2_90,
  &str2_2_91,
  &str2_2_92,
  &str2_2_93,
  &str2_2_94,
  &str2_2_95,
  &str2_2_96,
  &str2_2_97,
  &str2_2_98,
  &str2_2_99,
  &str2_2_100,
  &str2_2_101,
  &str2_2_102,
  &str2_2_103,
  &str2_2_104,
  &str2_2_105,
  &str2_2_106,
  &str2_2_107,
  &str2_2_108,
  &str2_2_109,
  &str2_2_110,
  &str2_10_16,
  &str2_10_17,
  &str2_10_18,
  &str2_13_20,
  &str2_13_21,
  &str2_13_22,
  &str2_13_23,
  &str2_13_24,
  &str2_13_25,
  &str2_13_26,
  &str2_1_11,
  &str2_1_12,
  &str2_1_13,
  &str2_1_14,
  &str2_3_20,
  &str2_3_21,
  &str2_3_22,
  &str2_3_23,
  &str2_8_50,
  &str2_8_51,
  &str2_8_52,
  &str2_8_53,
  &str2_8_54,
  &str2_8_55,
  &str2_8_56,
  &str2_8_57,
  &str2_9_48,
  &str2_9_49,
  &str2_9_50,
  &str2_9_51,
  &str2_9_52,
  &str2_9_53,
  &str2_12_36,
  &str2_12_37,
  &str2_12_38,
  &str2_12_39,
  &str2_12_40,
  &str2_12_41,
  &str2_4_58,
  &str2_4_59,
  &str2_4_60,
  &str2_4_61,
  &str2_4_62,
  &str2_4_63,
  &str2_4_64,
  &str2_4_65,
  &str2_5_149,
  &str2_5_150,
  &str2_5_151,
  &str2_5_152,
  &str2_5_153,
  &str2_5_154,
  &str2_5_155,
  &str2_5_156,
  &str2_5_157,
  &str2_5_158,
  &str2_5_159,
  &str2_5_160,
  &str2_6_72,
  &str2_6_73,
  &str2_6_76,
  &str2_7_143,
  &str2_7_144,
  &str2_7_145,
  &str2_7_146,
  &str2_7_147,
  &str2_7_148,
  &str2_7_149,
  &str2_7_150,
  &str2_7_151,
  &str2_7_152,
  &str2_7_153,
  &str2_7_154,
  &str2_7_155,
  &str2_7_156,
  &str2_14_143,
  &str2_14_144,
  &str2_14_145,
  &str2_14_146,
  &str2_14_147,
  &str2_14_148,
  &str2_14_149,
  &str2_14_150,
  &str2_14_151,
  &str2_14_152,
  &str2_14_153,
  &str2_14_154,
  &str2_14_155,
  &str2_14_156,
  &str2_14_157,
  &str2_14_158,
  &str2_14_159,
  &str2_14_160,
  &str2_14_161,
  &str2_14_162,
  &str2_2_111,
  &str2_6_77,
  &str2_6_78,
  &str2_6_79,
  &str2_6_80,
  &str2_6_81,
  &str2_6_82,
  &str2_7_157,
  &str2_7_158,
  &str2_7_159,
};

