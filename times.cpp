// MaxiM: Ported, not changed

#include "partoss.h"
#include "globext.h"

struct tm timetottmt(char *time);

unsigned long strtime(char *mtime)
{
  unsigned long ttime;
  unsigned short i,
    j;
  struct tm ttmt;

  ttmt = timetottmt(mtime);
//  memcpy(&ttime,&ttmt,szlong);
  i = (unsigned short)(ttmt.tm_hour);
  i <<= 6;
  i |= (unsigned short)(ttmt.tm_min);
  i <<= 5;
  i |= (unsigned short)(ttmt.tm_sec);
  j = (unsigned short)(ttmt.tm_year);
  j <<= 4;
  j |= (unsigned short)(ttmt.tm_mon);
  j <<= 5;
  j |= (unsigned short)(ttmt.tm_mday);
  ttime = i;
  ttime <<= 16;
  ttime += j;
  return (ttime);
}

/*
unsigned long strtime(char *time)
 {
  unsigned long ttime;
  unsigned short i,j;
  struct tm ttmt;
  ttmt=timetottmt(time);
  memcpy(&ttime,&ttmt,szlong);
  i=ttime;
  j=ttime>>16;
  ttime=i;
  ttime<<=16;
  ttime+=j;
  return(ttime);
 }
*/

short diffdays(unsigned long a, unsigned long b)
{
  short i;
  unsigned short ayear,
    amonth,
    aday,
    ahour,
    amin,
    atsec;
  unsigned short byear,
    bmonth,
    bday,
    bhour,
    bmin,
    btsec;

  ayear = (unsigned short)((a >> 9) & 0x7f);
  amonth = (unsigned short)((a >> 5) & 0x0f);
  aday = (unsigned short)(a & 0x1f);
  ahour = (unsigned short)(a >> 27);
  amin = (unsigned short)((a >> 21) & 0x3f);
  atsec = (unsigned short)((a >> 16) & 0x1f);
  byear = (unsigned short)((b >> 9) & 0x7f);
  bmonth = (unsigned short)((b >> 5) & 0x0f);
  bday = (unsigned short)(b & 0x1f);
  bhour = (unsigned short)(b >> 27);
  bmin = (unsigned short)((b >> 21) & 0x3f);
  btsec = (unsigned short)((b >> 16) & 0x1f);
  for(i = 0; i < 32767; i++)
  {
    if(ayear > byear)
      goto fixt;
    else if(ayear == byear && amonth > bmonth)
      goto fixt;
    else if(ayear == byear && amonth == bmonth && aday >= bday)
      goto fixt;
    aday++;
    switch (aday)
    {
      case 32:
	aday = 1;
	amonth++;
	break;

      case 31:
	if(amonth == 2 || amonth == 4 || amonth == 6 || amonth == 9
	    || amonth == 11)
	{
	  aday = 1;
	  amonth++;
	}
	break;

      case 30:
	if(amonth == 2)
	{
	  aday = 1;
	  amonth++;
	}
	break;

      case 29:
	if(amonth == 2 && ayear & 3)
	{
	  aday = 1;
	  amonth++;
	}
	break;

    }
    if(amonth > 12)
    {
      amonth = 1;
      ayear++;
    }
  }
fixt:
  if(i)
    if(ahour > bhour || (ahour == bhour && amin > bmin)
	|| (ahour == bhour && amin == bmin && atsec > btsec))
      i--;
  return i;
}

short converttime(char *time)
{
  // пpеобpазует стpоку time пpоизвольного фоpмата в стpоку по FTS-1
  struct tm ttmt;
  short result;

  ttmt = timetottmt(time);
  result = (ttmt.tm_mon > 0 && ttmt.tm_mon < 13 && ttmt.tm_mday > 0
	    && ttmt.tm_mday < 32 && ttmt.tm_hour >= 0 && ttmt.tm_hour < 24
	    && ttmt.tm_min >= 0 && ttmt.tm_min < 60 && ttmt.tm_sec <= 0
	    && ttmt.tm_sec < 31);
  if(ttmt.tm_year < 80)
    ttmt.tm_year += 80;
  if(ttmt.tm_mday < 10)
    ftstime[0] = '0';
  else
    ftstime[0] = (char)(ttmt.tm_mday / 10 + '0');
  ftstime[1] = (char)(ttmt.tm_mday % 10 + '0');
  ftstime[2] = ' ';
  memcpy(ftstime + 3, months[ttmt.tm_mon - 1], 3);
  ftstime[6] = ' ';
  while(ttmt.tm_year >= 100)
    ttmt.tm_year -= 100;
  if(ttmt.tm_year < 10)
    ftstime[7] = '0';
  else
    ftstime[7] = (char)(ttmt.tm_year / 10 + '0');
  ftstime[8] = (char)(ttmt.tm_year % 10 + '0');
  ftstime[9] = ' ';
  ftstime[10] = ' ';
  if(ttmt.tm_hour < 10)
    ftstime[11] = '0';
  else
    ftstime[11] = (char)(ttmt.tm_hour / 10 + '0');
  ftstime[12] = (char)(ttmt.tm_hour % 10 + '0');
  ftstime[13] = ':';
  if(ttmt.tm_min < 10)
    ftstime[14] = '0';
  else
    ftstime[14] = (char)(ttmt.tm_min / 10 + '0');
  ftstime[15] = (char)(ttmt.tm_min % 10 + '0');
  ftstime[16] = ':';
  if(ttmt.tm_sec < 5)
    ftstime[17] = '0';
  else
    ftstime[17] = (char)(ttmt.tm_sec / 5 + '0');
  ftstime[18] = (char)(ttmt.tm_sec % 5 + '0');
  ftstime[19] = 0;
  return result;
}

struct tm timetottmt(char *time)
{
  unsigned short i;
  struct tm ttmt;
  char *temp = NULL;

  temp = time;
  ttmt.tm_mday = 0;
  while(*temp)
  {
    while(isspace(*temp))
      temp++;
    if(isdigit(*temp))
    {
      i = (unsigned short)atoi(temp);
      while(isdigit(*temp))
	temp++;
      if(isspace(*temp))
      {
	if(ttmt.tm_mday || i > 31)
	{
	  if(i >= 1980)
	    i -= 1980;
	  else
	  {
	    if(i >= 80)
	      i -= 80;
	    else
	      i += 20;
	  }
	  ttmt.tm_year = i;
	}
	else
	  ttmt.tm_mday = i;
      }
      else
      {
	ttmt.tm_hour = i;
	while(!isdigit(*temp))
	  temp++;
	ttmt.tm_min = atoi(temp);
	while(isdigit(*temp))
	  temp++;
	while(!isdigit(*temp))
	  temp++;
	ttmt.tm_sec = atoi(temp) / 2;
      }
    }
    else
    {
      for(i = 0; i < 12; i++)
	if(memicmp(temp, months[i], 3) == 0)
	  break;
      if(i < 12)
	ttmt.tm_mon = i + 1;
    }
    while(*temp && !isspace(*temp++)) ;
  }
  return ttmt;
}
