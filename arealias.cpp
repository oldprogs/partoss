#include "partoss.h"
#include "globext.h"

void areaaliasmaker ()
{
  areaaliaslist.goto_head ();
  if (areaaliaslist.curr () == NULL)
    return;
  do
    {
      if (areaaliaslist.curr ()->data.linkaddr.zone == pktaddr.zone &&
	  areaaliaslist.curr ()->data.linkaddr.net == pktaddr.net &&
	  areaaliaslist.curr ()->data.linkaddr.node == pktaddr.node &&
	  areaaliaslist.curr ()->data.linkaddr.point == pktaddr.point)
	{
	  if (stricmp (areaaliaslist.curr ()->data.sarea, curarea) == 0)
	    {
	      memcpy (curarea, areaaliaslist.curr ()->data.darea, arealength);
	      return;
	    }
	}
    }
  while (areaaliaslist.next () != NULL);
};

char *areaaliasrestorer (char *carea)
{
  areaaliaslist.goto_head ();
  if (areaaliaslist.curr () == NULL)
    return NULL;
  do
    {
      if (areaaliaslist.curr ()->data.linkaddr.zone == tpack->outaddr.zone &&
	  areaaliaslist.curr ()->data.linkaddr.net == tpack->outaddr.net &&
	  areaaliaslist.curr ()->data.linkaddr.node == tpack->outaddr.node &&
	  areaaliaslist.curr ()->data.linkaddr.point == tpack->outaddr.point)
	{
	  if (stricmp (areaaliaslist.curr ()->data.darea, carea) == 0)
	    {
	      return areaaliaslist.curr ()->data.sarea;
	    }
	}
    }
  while (areaaliaslist.next () != NULL);
  return NULL;
};
