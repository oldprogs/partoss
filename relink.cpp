#include "partoss.h"
#include "globext.h"

struct uplname *uareas = NULL;
short badlink, manager = 0;

void relink (void)
{
  short i, j, k, l, numcomm, numofarc, temprescan, sort, linkchg =
    0, tupl, tnorep, isforw;
  unsigned short curr;
  char tempsqdn[(DirSize + 1)], tempserv[(DirSize + 1)], *temp =
    NULL, *temp2 =
    NULL, fname[DirSize + 1], ttoken[arealength + 1], wild[arealength + 1];
  struct uplname *areas = NULL, *tareas = NULL;
  struct uplname ttaname, *tname = NULL, *tuname = NULL;
  struct alists *ttlist = NULL;
  struct link *blink = NULL, tblink;
  struct packer arcdef;
  struct myaddr *taddr = NULL;
  numcomm = 31;
  rescandays = bcfg.rescdays;
  mystrncpy (tempserv, outbound, DirSize);
  mystrncat (tempserv, "tempserv.$$$", 16, DirSize);
  if ((tempsrv =
       (short)sopen (tempserv, O_RDWR | O_BINARY | O_CREAT, SH_DENYWR,
		     S_IRWXU | S_IRWXG | S_IRWXO)) == -1)
    {
      mystrncpy (errname, tempserv, DirSize);
      errexit (2, __FILE__, __LINE__);
    }
  chsize (tempsrv, 0);
  memset (&bufpkt, 0, szpackmess);
  bufpkt.packtype = 2;
  bufpkt.tozone = bufmess.fromzone;
  bufpkt.tonet = bufmess.fromnet;
  bufpkt.tonode = bufmess.fromnode;
  bufpkt.topoint = bufmess.frompoint;
  bufpkt.fromzone = bufmess.tozone;
  bufpkt.fromnet = bufmess.tonet;
  bufpkt.fromnode = bufmess.tonode;
  bufpkt.frompoint = bufmess.topoint;
  sftime = time (NULL);
  tmt = localtime (&sftime);
  mystrncpy (tstrtime, asctime (tmt), 39);
  converttime (tstrtime);
  mystrncpy (bufpkt.datetime, ftstime, 19);
  bufpkt.fromname = (char *)myalloc (36, __FILE__, __LINE__);
  bufpkt.toname = (char *)myalloc (36, __FILE__, __LINE__);
  bufpkt.subj = (char *)myalloc (72, __FILE__, __LINE__);
  mystrncpy (bufpkt.fromname, bcfg.manfrom, 35);
  mystrncpy (bufpkt.toname, bufmess.fromname, 35);
  mystrncpy (bufpkt.subj, bcfg.subj, 71);
  ptolen = strlen (bufpkt.toname);
  pfromlen = strlen (bufpkt.fromname);
  psubjlen = strlen (bufpkt.subj);
  bufpkt.flags = 0x181;
  pbigmess = 1;
  tail = (char *)myalloc (BufSize, __FILE__, __LINE__);
  sprintf (tail, "\r--- ParToss %s\r", version);
  addorig = 1;
  memcpy (&bufmess, &bufpkt, szpackmess);
  lseek (tempsqd, 0, SEEK_SET);

  tuname = bcfg.uplname;
  while (tuname)
    {
      isforw = 0;
      tlist = rlist;
      while (tlist)
	{
	  for (carea = 0; carea < tlist->numlists; carea++)
	    {
	      lseek (areaset, tlist->alist[carea].areaoffs, SEEK_SET);
	      rread (areaset, newarea, szarea, __FILE__, __LINE__);
	      newarea->links.chain = newarea->links.last = NULL;
	      newarea->links.numelem = 0;
	      if (newarea->type == 1)
		{
		  for (i = 0; i < newarea->numlinks; i++)
		    {
		      rread (areaset, &tsnd, szmyaddr, __FILE__, __LINE__);
		      if (cmpaddr (&tsnd, &tuname->upaddr) == 0)
			{
			  sprintf (logout, "+%s\r", newarea->areaname);
			  mywrite (tempsrv, logout, __FILE__, __LINE__);
			  isforw = 1;
			}
		    }
		}
	    }
	  tlist = tlist->next;
	}
      if (isforw)
	{
	  bufmess.tozone = tuname->upaddr.zone;
	  bufmess.tonet = tuname->upaddr.net;
	  bufmess.tonode = tuname->upaddr.node;
	  bufmess.topoint = tuname->upaddr.point;
	  taddr = bcfg.address.chain;
	  while (taddr)
	    {
	      if (taddr->point || bufmess.topoint)
		{
		  if ((taddr->zone == bufmess.tozone) &&
		      (taddr->net == bufmess.tonet) &&
		      (taddr->node == bufmess.tonode))
		    break;
		}
	      else if (taddr->zone == bufmess.tozone)
		break;
	      taddr = taddr->next;
	    }
	  if (taddr == NULL)
	    taddr = bcfg.address.chain;
	  bufmess.fromzone = taddr->zone;
	  bufmess.fromnet = taddr->net;
	  bufmess.fromnode = taddr->node;
	  bufmess.frompoint = taddr->point;
	  bufmess.flags = 0x181;
	  mystrncpy (bufmess.fromname, bcfg.manffrom, 35);
	  mystrncpy (bufmess.toname, tuname->upname, 35);
	  mystrncpy (bufmess.subj, tuname->uppass, 71);
	  tnorep = noreport;
	  noreport = 0;
	  makemsg (tempsrv, "forwarding", "");
	  noreport = tnorep;
	  chsize (tempsrv, 0);
	  lseek (tempsrv, 0, SEEK_SET);
	}
      tuname = tuname->next;
    }

  myfree ((void **)&tail, __FILE__, __LINE__);
  addorig = 0;
  cclose (&tempsrv, __FILE__, __LINE__);
  unlink (tempserv);
  mbigmess = zbigmess;
  if (!(mode & 1024 || mbigmess))
    {
      cclose (&tempsqd, __FILE__, __LINE__);
      unlink (tempsqdn);
    }
  else
    tempsqd = 0;
}
