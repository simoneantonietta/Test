/*
 * Last revision:  Wed 22-Jul-2009 23:46 bushbo 
 * Version:  0.1
 * Copyright (c) 2009 by Brian O. Bush, http://kd7yhr.org
 * License: GNU GPL, version 3 or later; http://www.gnu.org/copyleft/gpl.html
 */

#ifndef _TINYXML_H_
#define _TINYXML_H_

/* Types of events: start element, end element, text, attr name, attr
   val and start/end document. Other events can be ignored! */
enum {
  EVENT_START = 0, /* Start tag */
  EVENT_END,       /* End tag */
  EVENT_TEXT,      /* Text */
  EVENT_NAME,      /* Attribute name */
  EVENT_VAL,       /* Attribute value */
  EVENT_END_DOC,   /* End of document */
  EVENT_COPY,      /* Internal only; copies to internal buffer */
  EVENT_NONE       /* Internal only; should never see this event */
};

/* An opaque handle to the tinyxml parser context itself; typical
   tinyxml lifecycle is new(), feed(), feed(),... and finally free() */
typedef struct tinyxml_t tinyxml_t;

/* Callback function can optionally return 0 to exit parsing of
   document */
tinyxml_t* tinyxml_new(int bufsz,
                       int (*callback)(int event, 
                                       const char* txt, int len,
                                       void* user), 
                       void* user);

/* Event in human readable form */
const char* tinyxml_event_str(int event);

/* Feed data into the parser; Returns true unless halted, which
   indicates you should notcall feed again.  */
int tinyxml_feed(tinyxml_t* tinyxml, const char* buf, int len);

/* Free a tinyxml parser context */
void tinyxml_free(tinyxml_t* tinyxml);

#endif /* _TINYXML_H_ */
