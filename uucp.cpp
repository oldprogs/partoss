#include "partoss.h"
#include "globext.h"

char *uukeys[] = {
  "From:",
  "To:",
  "Subject:",
  "Date:",
  "Lines:"
};

short numkeys = 5;

void uupcin(void)
{
  struct fname *tname = NULL;

  tname = bcfg.uuname;
  while(tname)
  {
    uuhnd = mysopen(tname->file, 0, __FILE__, __LINE__);
    endinput[0] = 0;
    while(!endinput[0])
    {
      readblock(uuhnd, 0);
      endblock[0] = 0;
      do
      {
	getstring(0);
	gettoken(0);
	for(i = 0; i < numkeys; i++)
	  if(memicmp(token, uukeys[i], strlen(uukeys[i])) == 0)
	    break;
	switch (i)
	{
	  case 0:
	    gettoken(0);
	    tokencpy(bufpkt.fromname, 35);
	    break;

	  case 1:
	    gettoken(0);
	    tokencpy(bufpkt.toname, 35);
	    break;

	  case 2:
	    gettoken(0);
	    tokencpy(bufpkt.subj, 71);
	    break;

	  case 3:
	    break;

	  case 4:
	    break;

	  case 5:
	    break;

	  case 6:
	    break;
	}
      }
      while(!endblock[0]);
    }
    cclose(&uuhnd, __FILE__, __LINE__);
    tname = tname->next;
  }
}
