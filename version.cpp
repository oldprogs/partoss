#include "partoss.h"
#include "globext.h"

void printversion(void)
{
  sprintf(logout, "The Parma Tosser version %s", version);
  ccprintf("%s\r\n", logout);
#ifdef __NT__
  SetConsoleTitle(logout);
#endif

#ifndef __VERSION__
#ifdef __BORLANDC__
#define __VERSION__ "Borland C/C++"
#else
#ifdef __WATCOMC__
#define __VERSION__ "Watcom C/C++"
#else
#define __VERSION__ "Unknown C/C++ compiler"
#endif
#endif
#endif

  ccprintf("Compiled on %s %s with %s\r\n", __DATE__, __TIME__, __VERSION__);
}