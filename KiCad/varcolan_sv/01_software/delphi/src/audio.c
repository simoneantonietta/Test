#ifdef __CRIS__
#include "audio_iside.c"
#elif defined __arm__
#include "audio_athena.c"
#elif defined __i386__
#include "audio_pc.c"
#elif defined __x86_64__
#include "audio_pc.c"
#else
#error
#endif

