#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <mem.h>

#include "SwapExec.Hhh"

unsigned long pascal SwapExec (char *FileNameToSwap,
			       char *ProgramToExecute,
			       char *CommandString,
			       unsigned Actions, unsigned EnvSeg);

unsigned long pascal LastSwapError (void);


extern void *_Int0Vector;
extern void *_Int1Vector;
extern void *_Int4Vector;
extern void *_Int5Vector;
extern void *_Int6Vector;

#define Command  "Nc"

typedef struct
{
  unsigned char M_Type;
  unsigned M_Owner;
  unsigned M_Size;
}
Pp;

unsigned char SavTable[256 * 4];

void main (void)
{
  unsigned long d;
  char buf[256];

  puts ("****** Before swapping **********");
  disable ();
  memcpy (SavTable, MK_FP (0, 0), 256 * 4);
  *(unsigned void **)MK_FP (0, 0 * 4) = _Int0Vector;
  *(unsigned void **)MK_FP (0, 4 * 4) = _Int4Vector;
  *(unsigned void **)MK_FP (0, 5 * 4) = _Int5Vector;
  *(unsigned void **)MK_FP (0, 6 * 4) = _Int6Vector;
  enable ();

  sprintf (buf, " /c %s", Command);
// sprintf(buf, "%s", Command);
  d = SwapExec ("TempFile.Tmp", getenv ("COMSPEC"),
// d = SwapExec("TempFile.Tmp", buf,
		buf, Ac_Dsk + Ac_Xms + Ac_Ems + Ac_Path + Ac_Used + Ac_CoEx,
		0);

  disable ();
  memcpy (MK_FP (0, 0), SavTable, 256 * 4);
  enable ();

  printf ("****** After swapping, Result=%lXh (%lu.) ,\"%s\" **********\n", d,
	  d, LastSwapError ());
}
