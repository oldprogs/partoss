// MaxiM: Ported, not changed

#include "partoss.h"
#include "globext.h"

void writehead (void)
{
  union pktheader pkth;
  sftime = time (NULL);
  mylocaltime (&sftime, &tmt);
  pkth.twop.fromzone = tpack->fromaddr.zone;
  pkth.twop.fromnet = tpack->fromaddr.net;
  pkth.twop.fromnode = tpack->fromaddr.node;
  pkth.twop.frompoint = tpack->fromaddr.point;
  pkth.twop.tozone = tpack->outaddr.zone;
  pkth.twop.tonet = tpack->outaddr.net;
  pkth.twop.tonode = tpack->outaddr.node;
  pkth.twop.topoint = tpack->outaddr.point;
  pkth.twop.auxnet = tpack->fromaddr.net;
  pkth.twop.origzone = tpack->fromaddr.zone;
  pkth.twop.destzone = tpack->outaddr.zone;
  pkth.twop.year =
    (unsigned short)((tmt.tm_year > 1900) ? (tmt.tm_year)
		     : (tmt.tm_year + 1900));
  pkth.twop.month = (unsigned short)tmt.tm_mon;
  pkth.twop.day = (unsigned short)tmt.tm_mday;
  pkth.twop.hour = (unsigned short)tmt.tm_hour;
  pkth.twop.minute = (unsigned short)tmt.tm_min;
  pkth.twop.second = (unsigned short)tmt.tm_sec;
  pkth.twop.baud = 0;
  pkth.twop.pkttype = 2;
  pkth.twop.prodcode = 0xFE;
  pkth.twop.product = 0;
  pkth.twop.serialno = 0;
  pkth.twop.serminor = 95;
  pkth.twop.cw = 1;
  pkth.twop.cwvalcopy = 0x100;
  if (tpack->password[0])
    memcpy (pkth.twop.password, tpack->password, 8);
  else
    memset (pkth.twop.password, 0, 8);
  memset (pkth.twop.specific, 0, 4);
  wwrite (tpack->handle, &pkth, sztwoplus, __FILE__, __LINE__);
  tpack->touched = 1;
}

short readhead (short handle, struct myaddr *from, struct myaddr *to)
{
  short result = 0, i;
  union pktheader pkth;
  if (rread (handle, &pkth, 58, __FILE__, __LINE__) < 58)
    return -1;
  if (pkth.twot.subtype == 2)
    {
      from->zone = pkth.twot.fromzone;
      from->net = pkth.twot.fromnet;
      from->node = pkth.twot.fromnode;
      from->point = pkth.twot.frompoint;
      to->zone = pkth.twot.tozone;
      to->net = pkth.twot.tonet;
      to->node = pkth.twot.tonode;
      to->point = pkth.twot.topoint;
      memcpy (ppassword, pkth.twot.password, 8);
      if (bcfg.passci)
	for (i = 0; i < 8; i++)
	  ppassword[i] = (char)toupper (ppassword[i]);
      result = 1;
    }
  else
    if (pkth.twop.cw
	&& (pkth.twop.cw ==
	    (((pkth.twop.cwvalcopy >> 8) +
	      (pkth.twop.cwvalcopy << 8)) & 0xffff)))
    {
      from->zone = pkth.twop.fromzone;
      from->net =
	(pkth.twop.fromnet == 65535u ? pkth.twop.auxnet : pkth.twop.fromnet);
      from->node = pkth.twop.fromnode;
      from->point = pkth.twop.frompoint;
      to->zone = pkth.twop.tozone;
      to->net = pkth.twop.tonet;
      to->node = pkth.twop.tonode;
      to->point = pkth.twop.topoint;
      if (from->zone == 0)
	from->zone = pkth.twop.origzone;
      if (to->zone == 0)
	to->zone = pkth.twop.destzone;
      memcpy (ppassword, pkth.twop.password, 8);
      if (bcfg.passci)
	for (i = 0; i < 8; i++)
	  ppassword[i] = (char)toupper (ppassword[i]);
      result = 2;
    }
  else
    {
      from->zone = pkth.two.fromzone;
      from->net = pkth.two.fromnet;
      from->node = pkth.two.fromnode;
      from->point = 0;
      to->zone = pkth.two.tozone;
      to->net = pkth.two.tonet;
      to->node = pkth.two.tonode;
      to->point = 0;
      memcpy (ppassword, pkth.two.password, 8);
      if (bcfg.passci)
	for (i = 0; i < 8; i++)
	  ppassword[i] = (char)toupper (ppassword[i]);
      result = 0;
    }
  memset (from->domain, 0, 8);
  memset (to->domain, 0, 8);
  return result;
}
