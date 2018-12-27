Si può pensare a generalizzare i parametri di questa interfaccia.

Immaginando di riuscire ad avere un unico fw per tutti i tipi, c'è il problema di definire il tipo dei parametri che il tool presenta all'installatore. Il tipo parametri dipende infatti dal tipo periferica radio, ma non è pensabile e conveniente avere tutti tipi differenti, anche perché andrebbero tutti ogni volta configurati nell'SC8R.

Si configura quindi un solo tipo di periferica, tipo "Interfaccia 868" o qualcosa del genere, dove il primo parametro indica la configurazione dell'interfaccia (es: "Contatti", "Optex VX", "Optex BX", ecc) e che il tool deve quindi poi prendere in considerazione per rappresentare il tipo dei parametri.

Visto poi che il sottotipo può evolvere nel tempo, il fw deve controllare anche che il sottotipo che si vuole programmare sia effettivamente gestito dalla versione corrente, altrimenti ricadere in un default (es: contatti) e ritrasmettero indietro, così che sul tool ci sia evidenza che i parametri non sono stati programmati. Ricordarsi che comunque di mezzo c'è sempre l'SC8R.

