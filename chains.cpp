// MaxiM: Ported, not changed

#include "partoss.h"
#include "globext.h"

#include "chains.h"
#include "lowlevel.h"

void addshort (struct shortchain *chain, unsigned short net,
	       unsigned short node)
{
  struct shortaddr *tsnb = NULL;
  if (chain->numelem == 0)
    {
      chain->chain =
	(struct shortaddr *)myalloc (szshortaddr, __FILE__, __LINE__);
      tsnb = chain->chain;
    }
  else
    {
      tsnb = chain->last;
      tsnb->next =
	(struct shortaddr *)myalloc (szshortaddr, __FILE__, __LINE__);
      tsnb = tsnb->next;
    }
  chain->last = tsnb;
  tsnb->next = NULL;
  tsnb->net = net;
  tsnb->node = node;
  tsnb->hide = 0;
  chain->numelem++;
}

void delshort (struct shortchain *chain)
{
  struct shortaddr *tmp = NULL, *tmp2 = NULL;
  tmp = chain->chain;
  if (chain->numelem)
    {
      while (tmp)
	{
	  tmp2 = tmp->next;
	  myfree ((void **)&tmp, __FILE__, __LINE__);
	  tmp = tmp2;
	}
      chain->chain = chain->last = NULL;
      chain->numelem = 0;
    }
}

short addaddr (struct addrchain *chain, struct myaddr *addr)
{
  struct myaddr *link = NULL;
  link = chain->chain;
  while (link)
    {
      if (cmpaddr (link, addr) == 0)
	{
	  if (!link->rdonly)
	    link->rdonly = addr->rdonly;
	  if (!link->passive)
	    link->passive = addr->passive;
	  if (!link->deny)
	    link->deny = addr->deny;
	  return 0;
	}
      link = link->next;
    }
  if (chain->numelem == 0)
    {
      chain->chain = (struct myaddr *)myalloc (szmyaddr, __FILE__, __LINE__);
      link = chain->chain;
    }
  else
    {
      link = chain->last;
      link->next = (struct myaddr *)myalloc (szmyaddr, __FILE__, __LINE__);
      link = link->next;
    }
  chain->last = link;
  chain->numelem++;
  memcpy (link, addr, szmyaddr);
  link->next = NULL;
  return 1;
}

void deladdr (struct addrchain *chain)
{
  struct myaddr *tmp = NULL, *tmp2 = NULL;
  tmp = chain->chain;
  if (chain->numelem)
    {
      while (tmp)
	{
	  tmp2 = tmp->next;
	  myfree ((void **)&tmp, __FILE__, __LINE__);
	  tmp = tmp2;
	}
      chain->chain = chain->last = NULL;
      chain->numelem = 0;
    }
}

void addname (struct namechain *chain, struct sysname *name)
{
  struct sysname *tname = NULL;
  if (chain->numelem == 0)
    {
      chain->chain =
	(struct sysname *)myalloc (szsysname, __FILE__, __LINE__);
      tname = chain->chain;
    }
  else
    {
      tname = chain->last;
      tname->next = (struct sysname *)myalloc (szsysname, __FILE__, __LINE__);
      tname = tname->next;
    }
  chain->last = tname;
  chain->numelem++;
  memcpy (tname, name, szsysname);
  tname->found = 0;
  tname->next = NULL;
}

void delname (struct namechain *chain)
{
  struct sysname *tmp = NULL, *tmp2 = NULL;
  tmp = chain->chain;
  if (chain->numelem)
    {
      while (tmp)
	{
	  tmp2 = tmp->next;
	  myfree ((void **)&tmp, __FILE__, __LINE__);
	  tmp = tmp2;
	}
      chain->chain = chain->last = NULL;
      chain->numelem = 0;
    }
}

void addlink (struct linkchain *chain, struct link *link)
{
  struct link *tlink = NULL;
  if (chain->numelem == 0)
    {
      chain->chain = (struct link *)myalloc (szlink, __FILE__, __LINE__);
      tlink = chain->chain;
    }
  else
    {
      tlink = chain->last;
      tlink->next = (struct link *)myalloc (szlink, __FILE__, __LINE__);
      tlink = tlink->next;
    }
  chain->last = tlink;
  chain->numelem++;
  memcpy (tlink, link, szlink);
  tlink->next = NULL;
}

void dellink (struct linkchain *chain)
{
  struct link *tmp = NULL, *tmp2 = NULL;
  tmp = chain->chain;
  if (chain->numelem)
    {
      while (tmp)
	{
	  tmp2 = tmp->next;
	  myfree ((void **)&tmp, __FILE__, __LINE__);
	  tmp = tmp2;
	}
      chain->chain = chain->last = NULL;
      chain->numelem = 0;
    }
}
