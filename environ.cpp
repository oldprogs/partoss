// MaxiM: Ported.

#include "partoss.h"
#include "globext.h"

#ifdef M_I386
#define intx86 int386
#else
#define intx86 int86
#endif

unsigned dvtest (void)
{
  unsigned ver = 0;
#ifdef __DOS__
  REGS regs;
  regs.w.cx = 0x4445;
  regs.w.dx = 0x5351;
  regs.w.ax = 0x2b01;
  intdos (&regs, &regs);
  if (regs.h.al == 0xff)
    return 0;
  ver = regs.w.bx;
  indv = (ver != 0);
#endif
  return ver;
}

#ifdef __DOS__
void apicall (unsigned param)
{
#ifdef __DOS__
  REGS regs;
  regs.w.ax = 0x101;
  intx86 (0x15, &regs, &regs);
  regs.w.ax = param;
  intx86 (0x15, &regs, &regs);
  regs.w.ax = 0x1025;
  intx86 (0x15, &regs, &regs);
#endif
}
#endif

void mtsleep (short sec)
{
  if (bcfg.vsleep)
    {
      ccprintf ("%c\b", indicator[indpos]);
      indpos++;
      indpos %= 4;
    }
#if defined( __OS2__ )
  DosSleep ((unsigned long)sec * 1000);
  //sleep (sec);
#elif defined( __NT__ )
  Sleep ((DWORD) sec * 1000);
#elif defined(__linux__) || defined(__FreeBSD__)
  sleep (sec);
#else
  int i = 0;
  if (indv)
    for (i = 0; i < (sec << 2); i++)
      apicall (0x1000);
  else
    sleep (sec);
#endif
}

void dvbegcrit (void)
{
#if defined(__DOS__)
  if (indv)
    apicall (0x101b);
#endif
}

void dvendcrit (void)
{
#if defined(__DOS__)
  if (indv)
    apicall (0x101c);
#endif
}
