/*-----------------------------------------------------------------------
   xmem.c -- Extended Dynamic Memory Control Module.
           Dr. Dobb's Journal, #154 August 1989
           Modified by A.Boshkin, October 1990 (named idlxmem.c)
           Modified by P.Dubner,  September 1991 (named xm.c)
           Included to InfoScope Turbo C Tool Box, September, 1991
                (modified & named xm.c + xm.h)
           Rebuild completely by P.Dubner,  March 1992
----------------------------------------------------------------------  */
/* ************************ INCLUDE FILE *****************************  */
#include <mem.h>
#include <stdio.h>
#include <process.h>

#include "xm.h"

/* ************************ GLOBAL VARIABLES ************************** */
/* �ᯮ�짮����� xm_Trace:
(NO_HEAPCTL) => ����� �맮�� malloc & free
(FREE_CTL)   => ��᫥������� � �ᯮ�짮������ hash-⠡����
(FULL_CTL)   => � �� + ����஫� �� ��������ﬨ � ࠭�� �᢮��������� ������
*/
unsigned int xm_Trace = FULL_CTL;	/* ��ࠢ����� �஢��� ����஢��   */
unsigned long xm_userMemory = 0L;	/* ��饥 ���-�� �����, �뤥������ */
				    /*          ���짮��⥫�            */
unsigned long xm_totalMemory = 0L;	/* �㬬�୮� ���-�� ����⮩ ����� */
				    /*  (+������ �� ᮡ�⢥��� �㦤�)  */
unsigned long xm_userAlloc = 0L;	/* ����饥 ������⢮ ����ᮢ ����� */
unsigned long xm_totalAlloc = 0L;	/* ��饥 ������⢮ ����ᮢ ����� */

unsigned int xm_last_error_code;

void HeapWng (unsigned int code, void *ptr);

void (*xm_heapWng) (unsigned int code, void *ptr) = HeapWng;

char *xm_wngText[] = XM_WNGTEXT;

/* ************************ VARIABLES **************************  */
/* ������ ��� ��᫥������� ����ᮢ �����                             */

#define hashsize    47		/* ������ hash-⠡����               */
#define bucketsize  10		/* ���-�� ����⮢ �� hash bucket   */

/* amount of extra allocation for overhead*/
#define OVHSIZE 2

/* fill character for overhead gap*/
#define FILLCHAR        '\xFF'


typedef struct alloc_entry
{				/* Allocated entry information    */
  size_t size;			/* size of the allocated area     */
  char *ptr;			/* pointer to the allocated area  */
  long alloc_no;		/* allocation number of the area  */
  int chksum;			/* checksum of a freed area or 0  */
  int freed;			/* 1 if freed, 0 otherwise        */
}
ALLOCATION;

typedef struct bucket
{
  struct bucket *next;		/* Pointer to next bucket when filled */
  int entries;			/* number of used entries             */
  ALLOCATION *alloc;		/* allocated entry array              */
}
BUCKET;

#define NOT_FREED       256

#define HASH_INDEX(p) (int)((unsigned long)p % hashsize)
BUCKET **ptrHash = NULL;	/* dynamic pointer hash table */
unsigned long hashVolume = 0L;

void HeapWng (unsigned int code, void *ptr)
{
  xm_last_error_code = code;
  fprintf (stderr, "\nWARNING: %s, addr: %p", xm_wngText[code], ptr);
}

/* =======================================================================
      Compute checksum of a given area
*/
int checksum (char *p, size_t size)
{
  int i;
  unsigned char res = 0;

  for (i = 0; i < size; i++)
    res ^= *p++;

  return (int)res;
}

/* =======================================================================
        ��������� 㪠��⥫� � hash-⠡���
*/
int storePtr (void *p,		/* pointer to be stored */
	      size_t b)		/* size of area         */
{
  BUCKET *bp, *bq;		/* bucket pointers     */
  int bno;			/* bucket/entry number */

  if (!ptrHash)
    {				/* �㦭� ������ ��� �᭮���� hash-⠡����?      */
      if ((ptrHash =
	   (BUCKET **) calloc (hashsize, sizeof (BUCKET *))) == NULL)
	return 0;
      hashVolume = 0;
      xm_totalMemory = hashsize * sizeof (BUCKET *);
      xm_totalAlloc++;
    }

  bno = HASH_INDEX (p);		/* ���᫨� �室 � hash-⠡���� */
  /* ������ ���� �� ��������� ���������� bucket */
  for (bq = bp = ptrHash[bno]; bp && bp->entries == bucketsize; bp = bp->next)
    bq = bp;
  /* ��᫥ ��室� �� 横�� ����� ���������, ��:                  */
  /*      (a) � �⨬ �室�� �� �� ࠡ�⠫� -                     */
  /*          � �⮬ ��砥 bp == NULL � bq == NULL;              */
  /*      (b) � �⨬ �室�� ࠡ�⠫� � ��������� �� bucket'� -   */
  /*          � �⮬ ��砥 bp == NULL, �� bq != NULL.            */
  /*      (c) ������ ����������� ���������� bucket -             */
  /*          � �⮬ ��砥 bp != NULL � bq != NULL.              */
  if (bp == NULL)
    {				/* ��砩 (a) ��� (b)?  */
      if ((bp = (BUCKET *) malloc (sizeof (BUCKET))) == NULL)
	return 0;
      hashVolume++;
      xm_totalMemory += sizeof (BUCKET);
      xm_totalAlloc++;
      bp->alloc = (ALLOCATION *) calloc (bucketsize, sizeof (ALLOCATION));
      if (bp->alloc == NULL)
	return 0;
      xm_totalMemory += bucketsize * sizeof (ALLOCATION);
      xm_totalAlloc++;
      bp->next = NULL;
      bp->entries = 0;
      if (bq == NULL)		/* ����� (a): ���� bucket - � ������ ᯨ᪠ */
	ptrHash[bno] = bp;
      else			/* ����� (b): ���� bucket - � ����� ᯨ᪠  */
	bq->next = bp;
    }
  /* ������ �������� 㪠��⥫� � ��祥 */
  bno = bp->entries++;
  bp->alloc[bno].ptr = p;
  bp->alloc[bno].size = b;
  bp->alloc[bno].freed = NOT_FREED;
  bp->alloc[bno].chksum = 0;
  bp->alloc[bno].alloc_no = xm_totalAlloc;

  return 1;

}				/*storePtr */

int checkGap (char *p, int size)
{
  int gap;

  for (gap = size; gap < size + OVHSIZE; ++gap)
    if (p[gap] != FILLCHAR)
      return 0;
  return 1;

}				/*checkGap */

void freeMem (void *p, int size)
{
  free (p);
  xm_totalMemory -= size;
  xm_totalAlloc--;
}

/**/
/* =======================================================================
        �������� ��᪠ �����, ����㥬��� 㪠��⥫�� 'p'
                        �� hash ⠡����
   ======================================================================*/
int deletePtr (char *p)
{
  BUCKET *bp, *bq;		/* �����⥫� ��� bucket'��        */
  int bno, i;			/* ����� bucket'� � entry        */
  size_t size;			/* ������ �᢮��������� �����    */

  if (!ptrHash)
    return 0;			/* ��! ��祣� ������!          */

  bno = HASH_INDEX (p);		/* ���᫨� �室 � hash-⠡���� */

  /* �ன����� �� bucket'�� � ���᪠� 㪠��⥫� 'p'    */
  for (bq = NULL, bp = ptrHash[bno]; bp; bp = bp->next)
    {
      for (i = 0; i < bp->entries; i++)
	if (bp->alloc[i].ptr == p)
	  {			/* ��, �� �㦭�?       */
	    size = bp->alloc[i].size;
	    /* �०�� �ᥣ� �஢�ਬ, �� gap-������� �� ����������, */
	    /* �.�. �஢�ਬ, �� �� ��ᠫ� �� �뤥������ ������.    */
	    if (!checkGap (p, size))
	      xm_heapWng (OVERWRITE_WNG, p);
	    switch (xm_Trace)
	      {
	      case FULL_CTL:
		if (bp->alloc[i].freed == NOT_FREED)
		  {
		    /* ����஫쭠� �㬬�: ��⮬ �㤥� �஢�����, */
		    /* �� ��ᠫ� �� � ���� ����� �᢮��������    */
		    bp->alloc[i].chksum = checksum (bp->alloc[i].ptr, size);
		    bp->alloc[i].freed = !NOT_FREED;
		  }
		else
		  xm_heapWng (FREEING_FREE_WNG, p);
		break;
	      case FREE_CTL:
		freeMem (p, size + OVHSIZE);
		if (--bp->entries == 0)
		  {
		    if (bq)	/* ���ࠢ�� ᯨ᮪ bucket'��    */
		      bq->next = bp->next;
		    else
		      ptrHash[bno] = bp->next;
		    freeMem (bp->alloc, bucketsize * sizeof (ALLOCATION));
		    freeMem (bp, sizeof (BUCKET));
		    hashVolume--;
		  }		/*if(--bp->entries == 0) */
		else if (i < bp->entries)
		  bp->alloc[i] = bp->alloc[bp->entries];
		break;
	      default:		/* �������, ������� �� �����������      */
		fprintf (stderr, "\n\n\n�訡�� �� ࠡ�� ����� XM !!!\n\n");
		abort ();
	      }
	    return size;	/* ��ଠ�쭮� �����襭�� ࠡ��� */
	  }			/* if ... == p */
      bq = bp;
    }				/* for bq = ... */

  if (bp == NULL)
    xm_heapWng (FREEING_BAD_WNG, p);
  return 0;
}				/*deletePtr */

/* =======================================================================
        �뤥���� 'b' ���⮢ �����
*/
void *xm_Xmalloc (size_t b)
{
  char *mptr;

  if (xm_Trace != NO_HEAPCTL)
    {
      /* ������� ���祪 ��� ���쭥�襣� ����஫�     */
      if ((mptr = malloc (b + OVHSIZE)) != NULL)
	{
	  memset (mptr + b, FILLCHAR, OVHSIZE);	/* �������� �������     */
	  if (!storePtr (mptr, b))
	    {			/* 'mptr' - � ptrHash   */
	      free (mptr);
	      return NULL;
	    }
	  xm_userAlloc++;
	  xm_totalAlloc++;
	  xm_userMemory += b;
	  xm_totalMemory += b + OVHSIZE;
	}
    }
  else
    mptr = malloc (b);

  return mptr;

}				/*xm_Xmalloc */

/* =======================================================================
        Allocate and clear i*s bytes of memory
*/
void *xm_Xcalloc (unsigned int i,	/* �㦭�� ������⢮ ������   */
		  unsigned int s)	/* ������ ����� � �����        */
{
  register unsigned int amt;
  register char *mptr;

  if ((mptr = xm_Xmalloc (amt = i * s)) != NULL)
    memset (mptr, '\0', amt);	/* clear requested space        */

  return mptr;

}				/*xm_Xcalloc */


/* =======================================================================
        Free allocated memory
*/
void xm_Xfree (void *p)		/* 'p' points to block to be freed */
{
  if (p == NULL)
    xm_heapWng (FREEING_NULL_WNG, p);
  else if (xm_Trace != NO_HEAPCTL)
    {
      int size = deletePtr ((char *)p);
      if (size)
	{
	  xm_userMemory -= size;	/* ����४��㥬 ���稪�       */
	  xm_userAlloc--;
	}
    }
  else
    free (p);

}				/*xm_Xfree */

/* =======================================================================
        Check to ensure all blocks have been freed;
        free all the memory used if freeing is 1.
*/
int xm_XMem (int freeing, unsigned long start)
{
  register int bno, i;		/* bucket/entry number          */
  register BUCKET *bp, *bq;	/* bucket pointers              */
  int l;

  xm_last_error_code = 0;
  if (ptrHash == NULL || xm_Trace == NO_HEAPCTL)
    return 1;			/* ��! ��祣� ������!  */

  for (bq = NULL, bno = 0; bno < hashsize; ++bno)
    {
      for (bp = ptrHash[bno]; bp; bp = bp ? bp->next : NULL)
	for (l = bp->entries, i = 0; i < l; i++)
	  if (bp->alloc[i].alloc_no > start)
	    {
	      if (!checkGap (bp->alloc[i].ptr, bp->alloc[i].size))
		xm_heapWng (OVERWRITE_WNG, bp->alloc[i].ptr);
	      if (bp->alloc[i].freed == NOT_FREED)
		xm_heapWng (UNFREED_WNG, bp->alloc[i].ptr);
	      if (freeing)
		{
		  freeMem (bp->alloc[i].ptr, bp->alloc[i].size + OVHSIZE);
		  if (bp->alloc[i].freed == NOT_FREED)
		    {
		      xm_userAlloc--;
		      xm_userMemory -= bp->alloc[i].size;
		    }
		  l--;
		  if (--bp->entries == 0)
		    {
		      if (bq)	/* ���ࠢ�� ᯨ᮪ bucket'�� */
			bq->next = bp->next;
		      else
			ptrHash[bno] = bp->next;
		      freeMem (bp->alloc, bucketsize * sizeof (ALLOCATION));
		      freeMem (bp, sizeof (BUCKET));
		      hashVolume--;
		      bp = NULL;
		    }		/*if(--bp->entries == 0) */
		  else if (i < bp->entries)
		    bp->alloc[i] = bp->alloc[bp->entries];
		  break;
		}
	    }
      bq = bp;
    }

  if (hashVolume == 0)
    {
      free (ptrHash);
      ptrHash = NULL;
      xm_totalAlloc--;
      xm_totalMemory -= hashsize * sizeof (BUCKET *);
    }
  return xm_last_error_code == 0;
}				/*xm_XMem */
