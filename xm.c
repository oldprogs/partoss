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
/* Использование xm_Trace:
(NO_HEAPCTL) => простые вызовы malloc & free
(FREE_CTL)   => отслеживание с использованием hash-таблицы
(FULL_CTL)   => то же + контроль за изменениями в ранее освобожденных блоках
*/
unsigned int xm_Trace = FULL_CTL;	/* Управление уровнем трассировки   */
unsigned long xm_userMemory = 0L;	/* Общее кол-во памяти, выделенной */
				    /*          пользователю            */
unsigned long xm_totalMemory = 0L;	/* Суммарное кол-во занятой памяти */
				    /*  (+память на собственные нужды)  */
unsigned long xm_userAlloc = 0L;	/* Текущее количество запросов памяти */
unsigned long xm_totalAlloc = 0L;	/* Общее количество запросов памяти */

unsigned int xm_last_error_code;

void HeapWng (unsigned int code, void *ptr);

void (*xm_heapWng) (unsigned int code, void *ptr) = HeapWng;

char *xm_wngText[] = XM_WNGTEXT;

/* ************************ VARIABLES **************************  */
/* Таблица для отслеживания запросов памяти                             */

#define hashsize    47		/* Размер hash-таблицы               */
#define bucketsize  10		/* Кол-во элементов на hash bucket   */

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
        Запомнить указатель в hash-таблице
*/
int storePtr (void *p,		/* pointer to be stored */
	      size_t b)		/* size of area         */
{
  BUCKET *bp, *bq;		/* bucket pointers     */
  int bno;			/* bucket/entry number */

  if (!ptrHash)
    {				/* Нужна память под основную hash-таблицу?      */
      if ((ptrHash =
	   (BUCKET **) calloc (hashsize, sizeof (BUCKET *))) == NULL)
	return 0;
      hashVolume = 0;
      xm_totalMemory = hashsize * sizeof (BUCKET *);
      xm_totalAlloc++;
    }

  bno = HASH_INDEX (p);		/* Вычислим вход в hash-таблицу */
  /* Найдем первый не полностью заполненный bucket */
  for (bq = bp = ptrHash[bno]; bp && bp->entries == bucketsize; bp = bp->next)
    bq = bp;
  /* После выхода из цикла может оказаться, что:                  */
  /*      (a) с этим входом еще не работали -                     */
  /*          в этом случае bp == NULL и bq == NULL;              */
  /*      (b) с этим входом работали и заполнили все bucket'ы -   */
  /*          в этом случае bp == NULL, но bq != NULL.            */
  /*      (c) найден неполностью заполненный bucket -             */
  /*          в этом случае bp != NULL и bq != NULL.              */
  if (bp == NULL)
    {				/* случай (a) или (b)?  */
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
      if (bq == NULL)		/* ситуация (a): новый bucket - в голову списка */
	ptrHash[bno] = bp;
      else			/* ситуация (b): новый bucket - в конец списка  */
	bq->next = bp;
    }
  /* Теперь запомним указатель и прочее */
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
        Удаление куска памяти, адресуемого указателем 'p'
                        из hash таблицы
   ======================================================================*/
int deletePtr (char *p)
{
  BUCKET *bp, *bq;		/* Указатели для bucket'ов        */
  int bno, i;			/* Номера bucket'а и entry        */
  size_t size;			/* Размер освобождаемой памяти    */

  if (!ptrHash)
    return 0;			/* Ура! Нечего делать!          */

  bno = HASH_INDEX (p);		/* Вычислим вход в hash-таблицу */

  /* Пройдемся по bucket'ам в поисках указателя 'p'    */
  for (bq = NULL, bp = ptrHash[bno]; bp; bp = bp->next)
    {
      for (i = 0; i < bp->entries; i++)
	if (bp->alloc[i].ptr == p)
	  {			/* То, что нужно?       */
	    size = bp->alloc[i].size;
	    /* Прежде всего проверим, что gap-область НЕ изменилась, */
	    /* т.е. проверим, что не писали ЗА выделенную память.    */
	    if (!checkGap (p, size))
	      xm_heapWng (OVERWRITE_WNG, p);
	    switch (xm_Trace)
	      {
	      case FULL_CTL:
		if (bp->alloc[i].freed == NOT_FREED)
		  {
		    /* Контрольная сумма: потом будем проверять, */
		    /* не писали ли в блок ПОСЛЕ освобождения    */
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
		    if (bq)	/* Поправим список bucket'ов    */
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
	      default:		/* Надеюсь, НИКОГДА не понадобится      */
		fprintf (stderr, "\n\n\nОшибка при работе модуля XM !!!\n\n");
		abort ();
	      }
	    return size;	/* Нормальное завершение работы */
	  }			/* if ... == p */
      bq = bp;
    }				/* for bq = ... */

  if (bp == NULL)
    xm_heapWng (FREEING_BAD_WNG, p);
  return 0;
}				/*deletePtr */

/* =======================================================================
        Выделить 'b' байтов памяти
*/
void *xm_Xmalloc (size_t b)
{
  char *mptr;

  if (xm_Trace != NO_HEAPCTL)
    {
      /* Добавим кусочек для дальнейшего контроля     */
      if ((mptr = malloc (b + OVHSIZE)) != NULL)
	{
	  memset (mptr + b, FILLCHAR, OVHSIZE);	/* Заполним добавку     */
	  if (!storePtr (mptr, b))
	    {			/* 'mptr' - в ptrHash   */
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
void *xm_Xcalloc (unsigned int i,	/* Нужное количество блоков   */
		  unsigned int s)	/* Размер блока в байтах        */
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
	  xm_userMemory -= size;	/* Скорректируем счетчики       */
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
    return 1;			/* Ура! Нечего делать!  */

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
		      if (bq)	/* Поправим список bucket'ов */
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
