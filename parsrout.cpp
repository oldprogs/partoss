// MaxiM: memicmp

#include "partoss.h"
#include "globext.h"
#include "globfunc.h"

#include "parsrout.h"
#include "lowlevel.h"
#include "morfiles.h"
#include "chains.h"

void runroute (void)
{
  struct route *trou = NULL;
  struct myaddr tsnd;
  char hfile[DirSize + 1];
  addhome (hfile, bcfg.route);
  setf = mysopen (hfile, 0, __FILE__, __LINE__);
  mystrncpy (confile, hfile, DirSize);
  lineno[0] = 1;
  endinput[0] = 0;
  while (!endinput[0])
    {
      readblock (setf, 0);
      endblock[0] = 0;
      do
	{
	  getstring (0);
	  gettoken (0);
	  if (*token != ';')
	    {
	      memcpy (&defaddr, bcfg.address.chain, szmyaddr);
	      if (memicmp (token, "Route", 5) == 0)
		{
		  gettoken (0);
		  if (isdigit (token[0]) || token[0] == '/' || token[0] == ':'
		      || token[0] == '.')
		    {
		      if (roulist == NULL)
			{
			  roulist =
			    (struct route *)myalloc (szroute, __FILE__,
						     __LINE__);
			  trou = roulist;
			}
		      else
			{
			  trou = roulist;
			  while (trou->next)
			    trou = trou->next;
			  trou->next =
			    (struct route *)myalloc (szroute, __FILE__,
						     __LINE__);
			  trou = trou->next;
			}
		      memset (trou, 0, szroute);
		      trou->next = NULL;
		      parseaddr (token, &tsnd, toklen);
		      if (tsnd.zone)
			addaddr (&(trou->list), &tsnd);
		      while (!endstring[0])
			{
			  gettoken (0);
			  parseaddr (token, &tsnd, toklen);
			  if (tsnd.zone)
			    addaddr (&(trou->list), &tsnd);
			}
		    }
		}
	      else
		{
		  if (memicmp (token, "Direct", 6) == 0)
		    {
		      gettoken (0);
		      if (direct == NULL)
			{
			  direct =
			    (struct route *)myalloc (szroute, __FILE__,
						     __LINE__);
			  memset (direct, 0, szroute);
			  direct->next = NULL;
			}
		      parseaddr (token, &tsnd, toklen);
		      if (tsnd.zone)
			addaddr (&(direct->list), &tsnd);
		      while (!endstring[0])
			{
			  gettoken (0);
			  parseaddr (token, &tsnd, toklen);
			  if (tsnd.zone)
			    addaddr (&(direct->list), &tsnd);
			}
		    }
		  else
		    {
		      if (memicmp (token, "NoTouch", 7) == 0)
			{
			  gettoken (0);
			  if (notouch == NULL)
			    {
			      notouch =
				(struct route *)myalloc (szroute, __FILE__,
							 __LINE__);
			      memset (notouch, 0, szroute);
			      notouch->next = NULL;
			    }
			  parseaddr (token, &tsnd, toklen);
			  if (tsnd.zone)
			    addaddr (&(notouch->list), &tsnd);
			  while (!endstring[0])
			    {
			      gettoken (0);
			      parseaddr (token, &tsnd, toklen);
			      if (tsnd.zone)
				addaddr (&(notouch->list), &tsnd);
			    }
			}
		      else
			{
			  tokencpy (logout, BufSize);
			  if (strlen (logout))
			    ccprintf
			      ("Incorrect keyword \"%s\" in %s (line %d)\r\n",
			       logout, bcfg.route, lineno[0]);
			}
		    }
		}
	    }
	  lineno[0] += numcr[0];
	}
      while (!endblock[0]);
    }
  cclose (&setf, __FILE__, __LINE__);
}

short compaddr (struct myaddr *chain, struct myaddr *mess)
{
  struct myaddr *current = NULL;
  current = chain;
  while (current != NULL)
    {
      if ((current->zone == mess->zone || current->zone == 65535u) &&
	  (current->net == mess->net || current->net == 65535u) &&
	  (current->node == mess->node || current->node == 65535u) &&
	  (current->point == mess->point || current->point == 65535u))
	return 0;
      current = current->next;
    }
  return 1;
}
