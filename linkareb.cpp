// MaxiM: Slegka portirovana

/**************************************************************************
 * File: linkarea.cpp
 *
 * Copyright (C) 1998 by Vadim Belman (voland@plab.ku.dk)
 * Licensed for use in The Parma Tosser by Serge Koghin.
 *
 * Links a Squish(TM)-style message base in four different ways with the
 * highest possible speed.
 *
 **************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
// #include <math.h>
#include <ctype.h>
#include <fcntl.h>
#if ! ((defined(__linux__) && !defined(__EMX__)) || defined (__FreeBSD__))
#include <io.h>
#else
#include <sys/param.h>
#endif
#include <sys/timeb.h>
#include <signal.h>
#include "partoss.h"
#include "globext.h"

//#define MALLOC_DEBUG
// LINK_PROFILE ����砥� �p�䨫�p������ �� �p����� �ᯮ������.
//#define LINK_PROFILE
//#define STANDALONE_VERSION
// TRAP_CATCHING ���� ��� �������, �᫨ �p���室�� ����� �� �p�稭�
// ��� p��� �������, �p������� � ����p�p������ ᨣ�����. H��p���p -
// �p� �뫥� �� SIGSEGV.
//#define TRAP_CATCHING
#if defined( STANDALONE_VERSION) && !defined( MALLOC_DEBUG)
#define USE_SYSALLOC
#endif

#define LINKTYPE_REPLY          1
#define LINKTYPE_SUBJ           2
#define LINKTYPE_FLAT           3
#define LINKTYPE_TIMED          4
#define LINKTYPE_NONE           5

// ����� �᫮ ��������� �� ���祭�� �� ���䨣� �� ���ࠨ����� �
// �����. bcfg.linklength, ⠪ �������.
#define LINK_LENGTH             bcfg.linklength
// �᫨ subj � ��������� ����� �� ����稢����� 0, � ����� ���� ������ 72
// H� ⮣�� - ������ ���� ���� � ����.
#define SUBJ_MAXLEN             71

#if defined( __DOS__) && !defined( __386__)
// ��� 16bit p����p� ��襩 ���� �������, �� �p������ � ������让 ���p� ᪮p���,
// �� ������ ���������.
#define HASH_MULTIPLIER 1
#else
#define HASH_MULTIPLIER 3
#endif

// myalloc � myfree - �६���� �����誨, �⮡� �� �ਢ�뢠���� � ������ ���栬.
// USE_SYSALLOC �������, �⮡� �� �������p��� ����� p�� �� define'�.
// � p���祬 �p���� ��� ����� �p��� �� ��p�������.
#if defined( MALLOC_DEBUG)
#define myalloc( sz, f, l)  dbg_malloc( ( sz), ( f), ( l))
#define myfree( pptr, f, l) dbg_free( ( pptr), ( f), ( l))
#define myrealloc( ptr, sz, f, l) dbg_realloc( ( ptr), ( sz), ( f), ( l))
#elif defined( USE_SYSALLOC)
#define myalloc( sz, f, l)  malloc( sz)
#define myfree( pptr, f, l) free( *( pptr))
#define myrealloc( ptr, sz, f, l) realloc( ( ptr), ( sz))
#endif

#ifdef STANDALONE_VERSION

#define LINKFVERBOSESTEP    0
#define AREANAME            "UNKNOWN.NAME.FOR.DEBUG"

#ifdef TRAP_CATCHING
int last_line_passed = -1;
#define SETLINE         { last_line_passed = __LINE__;}
#else
#define SETLINE
#endif

#else

#define LINKFVERBOSESTEP    bcfg.linkverb
#define AREANAME            newarea->areaname

#define SETLINE

#endif

//#ifdef __WATCOM10_6__
//#define ccprintf              cprintf
//#else
//#define ccprintf              printf
//#endif

// O_BINARY ����� ���� �� ��।���� ��� ��-DOS-derived ������⥪ (��࠭�஢���� -
// � libc BSD-ᥬ���⢠, ���ਬ��). �⮡� �� ����� ᥡ� ����� ��⮬ - ��।����
// ��� �������.
#ifndef O_BINARY
#define O_BINARY            0x0000
#endif

// �� �� 䨣�� - � _MAX_PATH. ���� ��६ ��।������ �� BSD'譮�� libc.
// ��� ����� �㤥�.
#ifndef _MAX_PATH
#define _MAX_PATH           MAXPATHLEN
#endif

// �ᯮ������ ��� �஢�ન 楫��⭮�� ��������� �३�� � .SQD
#define SQHDRID       0xafae4453L

// ���-����� ������� ��p������...
static unsigned long changed_count = 0;

#ifdef MALLOC_DEBUG

static unsigned long mem_alloced = 0, max_alloced = 0;

#pragma pack( 1)
typedef struct
{
  unsigned long size;
  char blk[];
}
dbg_memblk;
#pragma pack()

void dbg_prtstat ()
{
  fprintf (stderr, "Memory: %-8lu / %-8lu\r", mem_alloced, max_alloced);
}

void *dbg_malloc (size_t blksize, const char *file_name, unsigned file_line)
{
  dbg_memblk *p = (dbg_memblk *) malloc (blksize + sizeof (unsigned long));

  if (p == NULL)
    {
      fprintf (stderr, "Allocation of %ld bytes failed at line %u of %s\n",
	       blksize, file_line, file_name);
      exit (1);
    }

  mem_alloced += blksize;
  if (max_alloced < mem_alloced)
    {
      max_alloced = mem_alloced;
    }
  p->size = blksize;

  dbg_prtstat ();

  return (void *)p->blk;
}

void *dbg_realloc (void *ptr, size_t blksize, const char *file_name,
		   unsigned file_line)
{
  dbg_memblk *p = NULL;

  if (ptr != NULL)
    {
      p = (dbg_memblk *) ((char *)ptr - sizeof (unsigned long));
      mem_alloced -= p->size;
    }

  p = (dbg_memblk *) realloc (p, blksize + sizeof (unsigned long));
  if (p == NULL)
    {
      fprintf (stderr, "Reallocation failed (%ld bytes) at line %u of %s\n",
	       blksize, file_line, file_name);
      exit (1);
    }

  mem_alloced += blksize;
  if (max_alloced < mem_alloced)
    {
      max_alloced = mem_alloced;
    }
  p->size = blksize;

  dbg_prtstat ();

  return (void *)p->blk;
}

void dbg_free (void **ptr, const char *file_name, unsigned file_line)
{
  dbg_memblk *p;

  if (ptr == NULL)
    {
      fprintf (stderr,
	       "Can't free NULL pointer to pointer at line %u of %s\n",
	       file_line, file_name);
      exit (1);
    }
  if (*ptr == NULL)
    {
      fprintf (stderr, "Can't free NULL pointer at line %u of %s\n",
	       file_line, file_name);
      exit (1);
    }

  p = (dbg_memblk *) ((char *)(*ptr) - sizeof (unsigned long));
  mem_alloced -= p->size;

  dbg_prtstat ();

  free (p);

  *ptr = NULL;
}

#endif

#ifdef LINK_PROFILE
// time_diff � time_point ����� ���� �ᯮ�짮���� ��� ��䨫�஢����
// ��᪮� ����, ����� ��� �६��� ���� �� �����ᥪ㭤�.
float timeb_diff (struct timeb &t1, struct timeb &t2)
{
  float tfm, tft;
  tfm = (((float)t1.millitm) - ((float)t2.millitm)) / 1000.;
  tft = ((float)t1.time) - ((float)t2.time);
  return tfm + tft;
}

float time_point ()
{
  static struct timeb pt;
  struct timeb curt;
  float diff;

  ftime (&curt);

  diff = timeb_diff (curt, pt);

  memcpy (&pt, &curt, sizeof (struct timeb));

  return diff;
}

void PROFILE (const char *comment = NULL)
{
  if (comment == NULL)
    {
      time_point ();
    }
  else
    {
      fprintf (stderr, "%-8.4f: %s", time_point (), comment);
    }
}

#else

// �㤥� ��⠢���� �������p�� � ��� ⠬, ��� � ��砥 �p�䨫�p������
// �㦭� check-point'�.
#define PROFILE( x)

#endif

//=========================================================================
// Working code
//=========================================================================

// ��� ���������� ��� � ����⢥ ���� �����筮 �ᯮ�짮���� MSGID ᠬ
// �� ᥡ� - �� �����筮 ��砥�.

#pragma pack( 1)
typedef struct MsgInfo
{
  // ����⢥��� ��������� ���ᠣ�, �筥� - ����室��� ��� ��᪨
  char *subj;
  unsigned long replyto, nextreply[10];

  // ����᫥���饥 ������ �� ������ - �⮡ ����� ᥡ� ��⮬ �� �����
  // � �離�� �����ᮢ.
  long offset;
  unsigned long umsgid;
  // ������ �, �� ���� ����� �ਣ�������.
  unsigned long msgid, reply_id;
  char changed;
  char processed;
  // ����� ���� ��� ⨯�� �������� Flat/Reply � Subj. � ��饬 - ���
  // ��������, ��� �㦭� ᯨ᪨.
  MsgInfo *next, *prev;		// �����⥫� �� ᫥���饥 � �।��饥 ���쬠 �
  // 楯�窥.
  MsgInfo *head;		// �����⥫� �� ������ ᯨ᪠.
  MsgInfo *last;		// �����⥫� �� ��᫥���� ����� � ᯨ᪥.
  // ����⢨⥫�� ⮫쪮 � ��������� ����� ᯨ᪠.
  inline void mark_changed ()
  {
    if (!changed)
      {
	changed = 1;
	changed_count++;
      }
  }
}
 *PMsgInfo;

typedef struct HashElem
{
  PMsgInfo msg_info;
  HashElem *next;
}
 *PHashElem;

typedef struct Hash
{
  unsigned long size;		// ������ ⠡����.
  unsigned long used;		// ��᫮ �ᯮ�짮������ ᫮⮢.
  unsigned long dupcount;	// ��᫮ ᫮⮢ � ����� 祬 ����� ����⮬.
  unsigned long elements;	// ��᫮ ����ᥭ��� ����⮢.
  // ��᫥���� ��� ���� - ��� ��� ����⨪�.
  PHashElem *table;		// ����⢥��� ���-⠡���, ��� �� - ���ᨢ
  // 㪠��⥫�� �� ᯨ᪨ ����⮢.
  unsigned long (*hashFunc) (unsigned long, PMsgInfo);
  int (*cmpFunc) (PMsgInfo, PMsgInfo);
}
 *PHash;
#pragma pack()

//-------------------------------------------------------------------------
// ��᪨ ࠡ��� � ��襬
//-------------------------------------------------------------------------

// ������� ���⮣� �᫠ ��� ࠧ��� ���.
unsigned long gen_prime (unsigned long base)
{
  unsigned long i;
  // ��� ��᫠ �᪠�� �।� ���� �ᥫ, ���⮬� ������塞 ������
  // ��� ᫥���饥 ���⭮� ��� ��⠢�塞 ��� ����.
  unsigned long atLeast = base + (base % 2 ? 0 : 1);

  // ��ॡ�ࠥ� �������⮢. ����� ����� ��������� ��ॡ��� �����
  // ����� �ᥫ. ����⢥���, ��אַ� ������ ��� �� ������ �����
  // 100000 ���� �।��� ࠧ���� ����� ���� �ᥤ���� ~10.4.
  // �ࠢ�� ��� �������� �� ���祭�� 㦥 ~12.7, �� ������� ��� ⠪��
  // ����! 8)
  for (i = atLeast;; i += 2)
    {
      if ((i % 3) == 0)
	{
	  // �p���p塞�� �᫮ ������� �� 3 - ��� ��᫠ � ��p���p� ����⥫��.
	  continue;
	}
      // �᫨ �᫮ � ����� ����⥫� - � �� � ����� ���� �����⭮��
      // �⮣� �᫠.
//        unsigned long divisor = ( unsigned long)sqrt( i);
      unsigned long divisor = (unsigned long)i / 2;
      // ������塞 ����⥫� ��� ������襥 ᢥ��� ���祭��, ���஥ �����⨬�
      // ��� ��ॡ�� ����⥫��. �����⨬� - ����� �� ��⭮ �� 3, �� 2.
      // ����������� ����� �뫮 �� �� � ���� 5 ����⡮������, �� "�
      // ⠪ ������". ��� १���� divisor ����� ���� ���� ����� ����
      // �����⭮�� �஢��塞��� �᫠, �� �� 㦥 �� ���譮.
      divisor = (divisor / 3) * 3 + 2;
      // ��� �����࠭ ��� ࠧ ⠪��, �⮡� �ய�᪠�� ��, �� ��⭮ 2 � 3.
      // ��㣨�� ᫮����, ��� ��� �� ��⭮�� 2 � 3 �᫠ D �᫮ ����襥
      // D �� �ந�������� �� 2 ���⪠ �� ������� D �� 3 - �᫮ �� ��⭮�
      // 2 � 3. ������⥫��⢮ ��⠢����� � ����⢥ �ࠦ�����. 8)
      for (; divisor > 3; divisor -= 2 * (divisor % 3))
	{
	  // �᫨ ���� �஢��塞�� �᫮ ࠧ�������� ��� ���⪠, �����
	  // ��� ��� �� ���室��.
	  if ((i % divisor) == 0)
	    {
	      break;
	    }
	}
      // �� ����⥫� ��ॡ࠭�, � �᫮ �� ࠧ�������� ��楫�; ����� -
      // ���⮥.
      if (divisor < 3)
	{
	  break;
	}
    }
  return i;
}

PHash createHash (unsigned long expectedElements,
		  unsigned long (*hashFunc) (unsigned long, PMsgInfo),
		  int (*cmpFunc) (PMsgInfo, PMsgInfo))
{
  PHash newHash;
  // �ਥ��஢�筮 �᫮ ����⮢ � ���-⠡��� ��஥ ����襥 祬
  // ��������� �᫮ ����⮢ ���� ~20% �㡫���⮢ (���� _��_ ������).
  // ����� ᮮ�頥�, �� �������� ��䥪⨢����� ��� ���⨣����� ��
  // ��� ࠧ��� ࠢ��� ������-���� ���⮬� ���.
  unsigned long hashSize = gen_prime (expectedElements * HASH_MULTIPLIER);

  if (expectedElements < 1)
    {
      expectedElements = 1;
    }

  newHash = (PHash) myalloc (sizeof (Hash), __FILE__, __LINE__);
#ifdef MALLOC_DEBUG
  fprintf (stderr, "\nAllocated new hash\n");
#endif
  newHash->size = hashSize;
  newHash->dupcount = newHash->used = newHash->elements = 0;
  newHash->table =
    (PHashElem *) myalloc (hashSize * sizeof (PHashElem), __FILE__, __LINE__);
#ifdef MALLOC_DEBUG
  fprintf (stderr, "\nAllocated new hash table for %d elements\n", hashSize);
#endif
  newHash->hashFunc = hashFunc;
  newHash->cmpFunc = cmpFunc;
  memset (newHash->table, 0, hashSize * sizeof (PHashElem));

  return newHash;
}

void destroyHash (PHash hash)
{
  int i;

  for (i = 0; i < hash->size; i++)
    {
      PHashElem curelem = hash->table[i];
      if (curelem != NULL)
	{
	  PHashElem next;
	  do
	    {
	      next = curelem->next;
	      myfree ((void **)&curelem, __FILE__, __LINE__);
	      curelem = next;
	    }
	  while (curelem);
	}
    }

  myfree ((void **)&hash->table, __FILE__, __LINE__);
  myfree ((void **)&hash, __FILE__, __LINE__);
}

void insertElement (PHash hash, PMsgInfo msgInfo)
{
  PHashElem hashElem;
  unsigned long offset;

  hashElem = (PHashElem) myalloc (sizeof (HashElem), __FILE__, __LINE__);
  hashElem->msg_info = msgInfo;

  // ���饭�� � ���-⠡��� �ਭ����� ࠢ�� ����� �� ������� MSGID ��
  // ࠧ��� ⠡����. � ��⮬ 㭨���쭮�� MSGID, ���, ����� ������,
  // ������ᨬ��� �� ��⠫��� MSGID � ���� � ����� � ������ - ������
  // ��砩����, ⠪�� ��ਠ�� ���-�㭪樨 ���� ���쬠 �����娥 १�����,
  // ����� ������ ��� ���ࠨ���� � �窨 �७�� ��� ��䥪⨢����
  // ��।������ ����⮢, ⠪ � ᪮��� ࠡ���.
  offset = hash->hashFunc (hash->size, msgInfo);

  if (hash->table[offset] == NULL)
    {
      // �⬥砥�, �� �᫮ �ᯮ��������� ᫮⮢ ᥩ�� 㢥������.
      hash->used++;
    }
  else
    {
      hash->dupcount++;
    }
  // ���� � ��� �祩�� ᢮����� - � ⮣�� next ���� ��������. ����
  // ��� ���-� 㦥 ����� - � ⮣�� �� ���� ��� ��⠢�� ������ �����
  // � ������ ᯨ᪠.
  hashElem->next = hash->table[offset];
  hash->table[offset] = hashElem;
  hash->elements++;
}

// ���� �㦭��� ����� �� MSGID.
PMsgInfo findElement (PHash hash, PMsgInfo msgInfo)
{
  unsigned long offset = hash->hashFunc (hash->size, msgInfo);

  PHashElem elem = hash->table[offset];

  while (elem && (!hash->cmpFunc (elem->msg_info, msgInfo)))
    {
      elem = elem->next;
    }

  // �� ���� ������ ���� ���� �⢥��: �᫨ ⠪��� msgid � ᫮� ���,
  // ⠪ ��� � ������� ⠬ ����㤠. ����� ⮫쪮 � १���� ���� ࠧ��
  // ���-�㭪樨 �뫨 ����.
  return (elem == NULL ? NULL : elem->msg_info);
}

// Hash-�㭪樨 ��� p����� �� MSGID
unsigned long hfMSGID (unsigned long hashSize, PMsgInfo msgInfo)
{
  return msgInfo->msgid % hashSize;
}

int cmpMSGID (PMsgInfo m1, PMsgInfo m2)
{
  return m1->msgid == m2->msgid;
}

// Hash-�㭪樨 ��� ࠡ��� � umsgid
// �ᮡ�������: ����� ������, � ⠪ �������, �� umsgid == ᬥ饭��
// � ���ᨢ� �����ᮢ + 1. ������, ��᪮��� 㢥७���� � �⮬ ���,
// � ���� ����� ��� ��� ���᪠ �� ���, ��� �ࠪ��᪨ �����猪
// ��� � �⮬ ��� �ᯮ������� �������.
unsigned long hfumsgid (unsigned long hashSize, PMsgInfo msgInfo)
{
  return msgInfo->umsgid % hashSize;
}

int cmpumsgid (PMsgInfo m1, PMsgInfo m2)
{
  return m1->umsgid == m2->umsgid;
}

// Hash-�㭪樨 ��� ࠡ��� � Subj.

// �㭪�� �����頥� ������ � subj, �� ���ன �����稢����� �������騥
// �஡��� � 'Re:', �.�. ������, � ���ன ᮡ�⢥��� subj � ᨤ��.
int cleanSubjPos (const char *subj)
{
  int i = 0;
  // �᫨ subj ������ �����稢����� 0, � ��� 0 ����� ���� ᪨����.
  // ���⮬� i < 71 � �� 72. ���祬, �� � ����� subj ������ ����, �⮡�
  // �᫮��� �ࠡ�⠫�???
  while (i < SUBJ_MAXLEN && isspace (subj[i])
	 || (strnicmp (subj + i, "RE:", 3) == 0))
    {
      if (isspace (subj[i]))
	{
	  i++;
	}
      else
	{
	  i += 3;
	}
    }

  return i;
}

short subjLen (const char *subj, int *pos)
{
  *pos = cleanSubjPos (subj);
  int slen = strlen (subj + *pos);
  return (short)(slen < LINK_LENGTH ? slen : LINK_LENGTH);
}

unsigned long hfSubj (unsigned long hashSize, PMsgInfo msgInfo)
{
  int pos, blen;
  unsigned long crc;

  blen = subjLen (msgInfo->subj, &pos);
  crc = crc32block (msgInfo->subj + pos, (short)blen, (unsigned long)-1);

  return crc % hashSize;
}

int cmpSubj (PMsgInfo m1, PMsgInfo m2)
{
  int len1, pos1, len2, pos2;

  len1 = subjLen (m1->subj, &pos1);
  len2 = subjLen (m2->subj, &pos2);

  return (len1 == len2)
    && (strncmp (m1->subj + pos1, m2->subj + pos2, len1) == 0);
}

//-------------------------------------------------------------------------
// ��᪨ ࠡ��� � �����
//-------------------------------------------------------------------------

// str2MSGID - ������ strtoul. ����p�� ����砥���.
unsigned long str2MSGID (char *str)
{
  unsigned long idval = 0, m;
  int i;
  char *p = (str[0] == '\001' ? str + 1 : str);
  char *p1 = strchr (p, '\001'), *p2 = strchr (p, '\000');
  // p1 � p2 ����� �� ������訩 ᫥���騩 ����� - �᫨ ⠪�� ����, � �����
  // ��p���, ᮮ�. p ����� �� ��᫥���� ᨬ��� ������.
  p = ((p1 != NULL) && (p1 < p2) ? p1 : p2) - 1;
  for (i = (p - str), m = 1; (i >= 0) && isalnum (*p); i--, p--, m *= 16)
    {
      idval += m * (isalpha (*p) ? toupper (*p) - 'A' + 10 : *p - '0');
    }
  return idval;
}

// ���᪨���� MSGID �� �������.
unsigned long getMSGID (char *ctrl)
{
  char *p = strstr (ctrl, "\001MSGID:");
  if (!p)
    {
      return (unsigned int)0;
    }
  return str2MSGID (p);
}

// ���᪨���� REPLY �� �������.
unsigned long get_reply_id (char *ctrl)
{
  char *p = strstr (ctrl, "\001REPLY:");
  if (!p)
    {
      return (unsigned int)0;
    }
  return str2MSGID (p);
}

//-------------------------------------------------------------------------
// ��������� TimEd-�⨫�.
//-------------------------------------------------------------------------

void linkTimEd (PHash msghash, PHash umsghash, PMsgInfo msgs,
		unsigned long frames)
{
  unsigned long i, j;
  int firstempty, linked;
  MsgInfo tmpmsg;

  changed_count = 0;

  for (i = 0; i < frames; i++)
    {
      PMsgInfo rmsg;		// � �㥬 �᪠�� ���쬮, �� ���஥ ⥪�饥 - �⢥�.

      if ((LINKFVERBOSESTEP > 0) && (((i /*+ 1 */ ) % LINKFVERBOSESTEP) == 0))
	ccprintf ("%s: linked %-6lu of %-6lu, changed %-6lu\r", AREANAME, i,
		  frames, changed_count);

      if (msgs[i].nextreply[0] != 0)
	{
	  // �� ��直� ��砩 �஢�ਬ - ���४⭠ �� ��ࢠ� ��뫪�
	  // � nextreply � �窨 �७�� TimEd-⨯�. ��� ����� ���������
	  // �����४⭮� �᫨ ࠭�� �ᯮ�짮����� ��㣮� ⨯. ��⠫��
	  // nextreply �� �஢��塞 ��室� �� ᫥����� �।���������:
	  // �) �ॢ������� �������� ����;
	  // �) �� �� ������ �ࠢ��쭮 (������� 8);
	  // �) �������� �����४⭮�� - १���� �쥩-� �ਢ���,
	  //    ������� �� ���ன �� �㤥�;
	  // �) �������: �� ����� ����⭮ ��ᠤ��� ᪮���� ࠡ���.
	  tmpmsg.umsgid = msgs[i].nextreply[0];
	  PMsgInfo nextmsg = findElement (umsghash, &tmpmsg);
	  if ((!nextmsg) || (nextmsg->reply_id != msgs[i].msgid))
	    {
	      msgs[i].nextreply[0] = 0;
	      msgs[i].mark_changed ();
	    }
	}
      if (msgs[i].reply_id == 0)
	{
	  // �� ����� �� �⢥� - ⠪ 祣� � ��� �������?
	  continue;
	}
      tmpmsg.msgid = msgs[i].reply_id;
      rmsg = findElement (msghash, &tmpmsg);
      if (rmsg == NULL)
	{
	  // �㦭�� ���ᠣ� � ���� ��� - �⢠������.
	  if (msgs[i].replyto != 0)
	    {
	      // � ࠭��, ��宦�, �뫠 - replyto ��࠭����. ����� -
	      // ���뢠�� � 0 � ������ ������.
	      msgs[i].replyto = 0;
	      msgs[i].mark_changed ();
	    }
	  continue;
	}
      // � ⥯��� ���� �� ����: ���� �� ��� ���ᠣ� 㦥 �뫨 ᫨�������,
      // ���� �� �� �।�⮨� ᤥ���� - � ⮣�� �㦭� ���� � nextreply
      // �⮡� � ���� �ய���� ��뫪�.
      firstempty = -1;
      linked = 0;
      if (msgs[i].replyto != rmsg->umsgid)
	{
	  // �������, ���ᠣ� �� �� �����������. ��� ��-� �����
	  // ���������. ��� ࠭�� ����������� ����. � �� ��砥 -
	  // ���⠢�塞 replyto.
	  msgs[i].replyto = rmsg->umsgid;
	  msgs[i].mark_changed ();
	}
      // ������ ����塞 ��� 䠪� ᫨���������� ���ᠣ, ��� ���⮯��������
      // ��� ��뫪� � nextreply. ��-�p������ �������� ����� ���p�p�������
      // nextreply, �� � ���筮� p����, ��宦�, �� �㤥� ���p��筮. � �p�����
      // ��砥 - ����� �㤥� "�p�ᮢ���" �p�p���� �p� �����. ��᪮���
      // � ���筮� ��砥 ������� �㤥� ᮢᥬ ������讥 �᫮ ���ᠣ,
      // � ��� ����奤 �� ������ ���� �����. ������ ���஢�� ࠭�� �����
      // �� �⮨� �� ᮮ�ࠦ���� ᪮��� (祬 ����� 横��� ���塞 - ⥬
      // ��� ���� 8).
      for (j = 0; j < 10; j++)
	{
	  if (msgs[i].umsgid == rmsg->nextreply[j])
	    {
	      // ����� �த������ ��� ��᫠ - ��� "�裡��� 㦥 �뫨", �
	      // ����� ������...
	      linked = 1;
	      break;
	    }
	  if (firstempty < 0 && rmsg->nextreply[j] == 0)
	    {
	      firstempty = j;
	    }
	}
      if (!linked && firstempty >= 0)
	{
	  rmsg->nextreply[firstempty] = msgs[i].umsgid;
	  rmsg->mark_changed ();
	}
      // ��⮢�. � �⮬� ������� �� ���易��...
    }
  if (LINKFVERBOSESTEP > 0)
    ccprintf ("%s: linked %-6lu of %-6lu, changed %-6lu\r", AREANAME, i,
	      frames, changed_count);
}

//-------------------------------------------------------------------------
// ��������� �������� ⨯�� �������� ��� @MSGID/@REPLY.
//-------------------------------------------------------------------------

// �஡���� �� ��������� ���쬠 �� ������ � ��뢠��� ���ᠣ � ᯨ᮪ �⢥⮢.
// �।����������, �� �������� ���쬮 ���� ������� ᯨ᪠.
void markList (PHash umsghash, PMsgInfo msg)
{
  PMsgInfo curMsg = msg;
  MsgInfo tmpmsg;
  msg->head = msg;
  msg->last = msg;
  while (curMsg && (curMsg->nextreply[0] != 0))
    {
      tmpmsg.umsgid = curMsg->nextreply[0];
      curMsg->next = findElement (umsghash, &tmpmsg);
      if (curMsg->next)
	{
	  if (curMsg->next->reply_id == 0)
	    {
	      // ������ - ��� �� ᯨ�� �� ������ 楯�窨 �⢥⮢! ����
	      // ������ - �㡨�� ⠪�� ��뫪� � ����.
	      curMsg->nextreply[0] = 0;
	      curMsg->mark_changed ();
	      continue;
	    }
	  if (curMsg->next->prev == NULL)
	    {
	      curMsg->next->prev = curMsg;
	    }
	  else
	    {
	      // �� � � 横� ���⥫�, �� � �� ��㣮� ᯨ᮪ ��⪭㫨��.
	      //
	      // �� ��������� ���樨: � ��ࢮ� ��砥 prev � ����������
	      // ����� �� ����� �� curMsg. �᫨ �����, � �� �����
	      // ���� १���⮬ ��室� �� �����-� ���ᠣ� ����� �� 㯮�
	      // � ᡮન ��� ��ᥬ �� ��� � ᯨ᮪. ����� �㤥� ������
	      // markList ��� ��������� ������ ᯨ᪠, ����� ᮡᢥ��� ᠬ
	      // ᯨ᮪ 㦥 ����.
	      // �᫨ prev ����� �� �� curMsg, � �� ����� ���� ���
	      // 横� ⠪ � ��㣮� ᯨ᮪. ���᭨�� ����� �� ᮤ�ন����
	      // head: �᫨ ᮢ������ � curMsg->head, � 横�.
	      //
	      // �� ��஬ ��砥 next ���������� ���쬠 ����� �� curMsg.
	      // �� �����, �� ��� ���ᠣ� ���� �� ����� ��뫠����.
	      // ���� 横�, ����筮 ��. 8)
	      //
	      // � ���쥬 ��砥 � ���� ��뫪� �� ᥡ� ��.

	      if (curMsg->next->next == curMsg)
		{
		  curMsg->next->nextreply[0] = 0;
		}
	      if ((curMsg->next->prev != curMsg) ||
		  (curMsg->next->next == curMsg) || (curMsg->next == curMsg))
		{
		  curMsg->nextreply[0] = 0;
		  curMsg->mark_changed ();
		  continue;
		}
	    }
	  curMsg->next->head = msg;
	  curMsg->next->processed = 1;
	  // �� �������⭮ - ���� �� �த������� � ᯨ᪠, ⠪ �� ��
	  // ��直� ��砩 - �������� ��������� ���쬮 ��� ��᫥���� �
	  // ᯨ᪥. �᫨ ���� ��� nextreply[ 0] �� ����, � ����
	  // ⠪��� umsgid ��������� - ����� � ���� �����冷�, � ����
	  // ���� ������� �� �� �����. �᫨ ᯨ᮪ ���� �㤥� �த�������� -
	  // �����४⭮��� ��⮬�⮬ ��������.
	  msg->last = curMsg->next;
	}
      curMsg = curMsg->next;
    }
}

void linkLinear (PHash msghash, PHash umsghash, PMsgInfo msgs,
		 unsigned long frames, int linkType)
{
  PMsgInfo *fakemsgs;		// � �㤥� �����뢠�� 䨪⨢�� ���ᠣ� ��� ����,
  // ����� reply ����, � �ࣨ��� �襫. ����� �ਣ�����
  // �㤥� ᮧ�������� 䨪⨢ � ����娢����� � ���.
  // ��� ������ ����� ��ঠ�� �㤥� ���ᨢ 㪧�⥫��,
  // � 䨪⨢� ��।����� �� ����室�����.
  unsigned long fakesize, fakechunk;
  unsigned long fakecount = 0;	// ������⢮ ॠ�쭮 �������� 䨪⨢��� ���ᠣ.
  unsigned long i;
  MsgInfo tmpmsg;

  changed_count = 0;

  // �� �㬠�, �⮡� �᫮ 㡨��� ��ᥬ, �� ����� ���� �⢥��, �뫮 �����
  // �⢥�� �������� � ����. ���॥ �ᥣ� - �� �������� � 10-� ���.
  // �� �᫨ �� �� ���� ⠪��� ���ᨢ� �� 墠�� - �����।���� �� ����室�����.
  // fakechunk ������ ��� ���������� �����।������ �⮡� ����� ������
  // ᥡ� ������ - ������ ��᪠�� ���騢��� ���ᨢ 䥩���.
  if (linkType == LINKTYPE_REPLY)
    {
      fakesize = fakechunk = frames < 4 ? frames : frames / 4;
      fakemsgs =
	(PMsgInfo *) myalloc (sizeof (PMsgInfo) * fakesize, __FILE__,
			      __LINE__);
    }

  for (i = 0; i < frames; i++)
    {
      PMsgInfo rmsg, curMsg;

      if ((LINKFVERBOSESTEP > 0)
	  && (((i /* + 1 */ ) % LINKFVERBOSESTEP) == 0))
	ccprintf ("%s: linked %-6lu of %-6lu, changed %-6lu\r", AREANAME, i,
		  frames, changed_count);

      if (msgs[i].processed)
	{
	  // � �⮩ ���ᠣ�� 㦥 ��ࠡ�⠫� - ��� ��᫠ ��������.
	  continue;
	}
      msgs[i].processed = 1;
      if (msgs[i].reply_id == 0)
	{
	  // ����� ��������� ��⥭樠���� ������ ᯨ᪠. ����� ���
	  // �஢���� - � �� ᫨������� �� ��� 㦥 � ����७쪮 �஡�������
	  // �� ������, �஬�ન஢��� �� � ��ନ஢��� ᯨ᮪.
	  // ������ ������� ��� 㦥 �஫���������� ��ᥬ.
	  // �᫨ �p� �⮬ ��������, �� replyto ������, � ������ �뢮�
	  // � ⮬, �� �� ��⠫�� �� ��p�� ⨯�� �������� � ��⨬.
	  if (msgs[i].replyto != 0)
	    {
	      msgs[i].replyto = 0;
	      msgs[i].mark_changed ();
	    }
	  markList (umsghash, msgs + i);
	  continue;
	}
      // ���쬮 ���� �⢥⮬, ���⮬� ���� �஡������� ����� ��
      // reply-楯�窥, ���� �� ������ ������ ��� �� ��⪭���� �� 㦥 ����騩��
      // ᯨ᮪. � ��饬 ���. ���⭮�� - �� if'��. 8)
      curMsg = msgs + i;
      tmpmsg.msgid = curMsg->reply_id;
      rmsg = findElement (msghash, &tmpmsg);
      if (rmsg != NULL)
	{
	  while (rmsg)
	    {
	      if (rmsg->processed)
		{
		  // ���� - � ���� �� 㦥 ���騩�� ᯨ᮪.
		  // �㤥� �ਢ�뢠���� � ��� 墮���. curMsg ᥩ�� 㪠�뢠��
		  // �� ���� ����� � ����� ���楯�窥.
		  PMsgInfo head = rmsg->head;
		  PMsgInfo last = head->last;
		  last->next = curMsg;
		  curMsg->prev = last;
		  last->nextreply[0] = curMsg->umsgid;
		  last->mark_changed ();
		  curMsg->replyto = last->umsgid;
		  curMsg->mark_changed ();

		  // �⬥⨬ � ������ ᯨ᪠, �� ��᫥���� ����� ⥯���
		  // ��㣮�.
		  head->last = msgs + i;

		  // � ⥯��� �਩����� �஡������� �� curMsg ���� �� ������
		  // ���ᯨ�� � �ய���� ����� head.
		  while (curMsg)
		    {
		      curMsg->head = head;
		      curMsg = curMsg->next;
		    }

		  // � 墠�� � �⨬. ��� ������� � ᫥���饩 ���ᠣ�.
		  break;
		}

	      if (curMsg->replyto != rmsg->umsgid)
		{
		  // ���쬮 �� �� ����������� ��� �뫮 ᫨������� �����४⭮.
		  // �� ���� 䨪���.
		  curMsg->replyto = rmsg->umsgid;
		  curMsg->mark_changed ();
		}
	      if (rmsg->nextreply[0] != curMsg->umsgid)
		{
		  // ���� � �� 䨣��: ���� � nextreply ᨤ�� ���⪨
		  // ࠭�� �ᯮ�짮���襣��� ⨯� ��������, ���� ���쬮
		  // ����� �� �� �����࣠���� ��ࠡ�⪥.
		  rmsg->nextreply[0] = curMsg->umsgid;
		  rmsg->mark_changed ();
		}

	      // ���뢠�� ���� ��ᥬ � ⮯��� ����� �����.
	      curMsg->prev = rmsg;
	      rmsg->next = curMsg;

	      curMsg = rmsg;
	      curMsg->processed = 1;
	      if (curMsg->reply_id != 0)
		{
		  // �� �� �� �⢥� �� �����-� ��㣮� ���쬮. ���஡㥬
		  // ��� ����.
		  tmpmsg.msgid = curMsg->reply_id;
		  rmsg = findElement (msghash, &tmpmsg);
		}
	      else
		{
		  // � ��� ⥯��� ���ࠫ��� �� ������ ������ ᯨ᪠.
		  rmsg = NULL;
		}
	    }
	  // ����� � ��� �������� ��� ����. ���� rmsg �� NULL - �����
	  // ���� ���ᯨ᮪ 㦥 ������������ � 墮��� ����饣���. �����
	  // ��� ���室��� � ᫥���饩 ���ᠣ�.
	  // �᫨ �� rmsg == NULL, � ��諨 ���� ᯨ᮪ � curMsg ᥩ��
	  // ���� ��� �������. ����� ����� ����� ᥡ� ��� �᫨ ��
	  // �ࠧ� �� ��諨 �ਣ���� ��� msgs[ i].
	  if (rmsg != NULL)
	    {
	      continue;
	    }
	}
      // ����� curMsg - �⢥� ��� �ਣ����� ��� ������ ������ ᯨ᪠.
      // � ��ࢮ� ��砥 ����ᨬ �� ⨯� ��������.
      if (curMsg->replyto != 0)
	{
	  // ���⨬ �������� ���⪨ ��᫥ ᬥ�� ⨯� ��������.
	  // � ᠬ�� ����: ����� ����� ���� replyto � ������ ᯨ᪠?
	  curMsg->replyto = 0;
	  curMsg->mark_changed ();
	}
      if (linkType == LINKTYPE_FLAT || curMsg->reply_id == 0)
	{
	  // �᫨ Flat - � �ਭ����� ���ᠣ� �� ������ � ��ந� �� �⮩
	  // ������ ���� ᯨ᮪.
	  markList (umsghash, curMsg);
	}
      else
	{
	  // �᫨ REPLY - � ᮧ���� fake-���쬮 ��� �ਣ����� �
	  // ��� �믮���� ஫� ������ ������ ᯨ᪠.
	  if (fakecount == fakesize)
	    {
	      // ���᮪ fake'�� �����. ���� ���������� storage.
	      fakesize += fakechunk;
	      fakemsgs =
		(PMsgInfo *) myrealloc (fakemsgs,
					sizeof (PMsgInfo) * fakesize,
					__FILE__, __LINE__);
	    }
	  fakemsgs[fakecount] =
	    (PMsgInfo) myalloc (sizeof (MsgInfo), __FILE__, __LINE__);
	  memset (fakemsgs[fakecount], 0, sizeof (MsgInfo));

	  fakemsgs[fakecount]->processed = 1;
	  fakemsgs[fakecount]->msgid = curMsg->reply_id;

	  fakemsgs[fakecount]->nextreply[0] = curMsg->umsgid;

	  // ������ ��� 䨪⨢ �㦥� ��� � ���. ��筥� - ����� �����������
	  // �����, ��� �離� � �� @REPLY.
	  insertElement (msghash, fakemsgs[fakecount]);

	  markList (umsghash, fakemsgs[fakecount]);

	  fakecount++;
	}
    }
  if (LINKFVERBOSESTEP > 0)
    ccprintf ("%s: linked %-6lu of %-6lu, changed %-6lu\r", AREANAME, i,
	      frames, changed_count);

  // ����頥� �� ᮡ�� ������...
  if (linkType == LINKTYPE_REPLY)
    {
      for (i = 0; i < fakecount; i++)
	{
	  myfree ((void **)&(fakemsgs[i]), __FILE__, __LINE__);
	}
      myfree ((void **)&fakemsgs, __FILE__, __LINE__);
    }
}

//-------------------------------------------------------------------------
// ��������� �������� �� Subj
//-------------------------------------------------------------------------

void linkSubj (PMsgInfo msgs, unsigned long frames)
{
  unsigned long i;
  // ��� ��� ����� ᯨ᪮� �������� �� Subj.
  PHash smsghash;

  changed_count = 0;

  // �����뢠��, �� ᯨ᪮� ������ �� Subj �㤥� �� �����, 祬 ���� ��
  // ��饣� �᫠ ��ᥬ.
  smsghash = createHash (frames / 3, hfSubj, cmpSubj);

  for (i = 0; i < frames; i++)
    {
      PMsgInfo hmsg, curMsg;

      if ((LINKFVERBOSESTEP > 0)
	  && (((i /* + 1 */ ) % LINKFVERBOSESTEP) == 0))
	ccprintf ("%s: linked %-6lu of %-6lu, changed %-6lu\r", AREANAME, i,
		  frames, changed_count);

      if (msgs[i].processed)
	{
	  // ��� �� 㦥 ���뢠�� - ����� � �� �஢�����.
	  continue;
	}

      // ��⠥��� ���� ������ ᯨ᪠ ��ᥬ ��� subj ⥪�饣� ���쬠.
      // �᫨ ����� - ����� ���� ����� ����� ���� ᯨ᮪. ��� - �����
      // ��⠢�塞 ����� ���쬮 � 墮��.
      curMsg = msgs + i;
      curMsg->processed = 1;
      // ��p����� ��� Subj �������� - �����. ������ ���⮪ p���� �ᯮ�짮���襩��.
      if ((curMsg->nextreply[0] != 0)
	  && (curMsg->nextreply[0] <= curMsg->umsgid))
	{
	  curMsg->nextreply[0] = 0;
	  curMsg->mark_changed ();
	}
      hmsg = findElement (smsghash, curMsg);
      if (hmsg != NULL)
	{
	  PMsgInfo last = hmsg->last;

	  curMsg->head = hmsg;
	  curMsg->prev = last;
	  last->next = curMsg;

	  if (curMsg->replyto != last->umsgid)
	    {
	      // ���쬮 �� �� ����������� ��� �뫮 ᫨������� �����४⭮.
	      // �� ���� 䨪���.
	      curMsg->replyto = last->umsgid;
	      curMsg->mark_changed ();
	    }
	  if (last->nextreply[0] != curMsg->umsgid)
	    {
	      // ���� � �� 䨣��: ���� � nextreply ᨤ�� ���⪨
	      // ࠭�� �ᯮ�짮���襣��� ⨯� ��������, ���� ���쬮
	      // ����� �� �� �����࣠���� ��ࠡ�⪥.
	      last->nextreply[0] = curMsg->umsgid;
	      last->mark_changed ();
	    }

	  hmsg->last = curMsg;
	}
      else
	{
	  // ���᪠ � ⠪�� subj ��宦� ���. ����� �㤥� ��稭��� ����.

	  insertElement (smsghash, curMsg);

	  curMsg->head = curMsg;
	  curMsg->last = curMsg;
	  if ((curMsg->replyto != 0))
	    {
	      curMsg->replyto = 0;
	      curMsg->mark_changed ();
	    }

	  // H�䨣 � �� �����? �������� �� � ��p����� ��뫪� ���p��,
	  // �᫨ ��� ����... 8-\ �p����, �᫨ �⮣� �� ������, � �����
	  // ��⠢����� ����pp���� ��뫪� ���p�� �p� ᬥ�� ⨯�� ��������.
	  // H� ���� ⠪, 祬 ����� p�� �p����᪨ �� ��p������뢠��.
	  //curMsg->nextreply[ 0] = 0;

	  // ����� ������, ���樠������ head ��� �������� �� subj ������,
	  // �� ���� �� �����, �᫨ ����������� � ���饬.
	}
    }
  if (LINKFVERBOSESTEP > 0)
    ccprintf ("%s: linked %-6lu of %-6lu, changed %-6lu\r", AREANAME, i,
	      frames, changed_count);

  destroyHash (smsghash);
}

//-------------------------------------------------------------------------
// ��������� �������� �����... 8)
//-------------------------------------------------------------------------
// ����⢥��� �㭪�� ��� ��������. �ਭ����� ��� 䠩�� � ����� ��� ���७��
// � ⨯ ��������.
// ���ਬ��: \fido\sqbase\A0001112
// ����� ������� ������ 䠩��� \fido\sqbase\A0001112.sqd �
// \fido\sqbase\A0001112.sqi.
// �㭪�� �����頥� 0 �᫨ �� ��諮 �ᯥ譮 ��� ��� �訡��. ��㣮�
// ��ࠡ�⪨ �訡�� �।�ᬠ�ਢ����� �� �㤥� �� ������ ���ࠨ�����
// � partoss.
int linkArea (const char *areaFile, int linkType)
{
  sqifile *index;		// Storage ��� ������.
  long i;
  int hi, hd;			// ������ ��� 䠩����� ����権.
  // hi - ��� ������, hd - ��� ����.
  char index_file[_MAX_PATH], base_file[_MAX_PATH];
  struct stat idxst;		// � ������� ���ଠ�� � 䠩�� �����ᮢ.
  int rd;			// � �㤥� ����娢��� १���� read'�.
  unsigned long frames;		// ��᫮ �३��� � ����
  PMsgInfo msgs;		// ���ᨢ ������ � ���ᠣ��.
  PHash msghash, umsghash;	// ��� � ������஢����� �� MSGID � umsgid ᮮ�.
  unsigned long zero_array[10] = { 0 };
  // �⮣� �㤥� �ᯮ�짮���� �� �஢�થ ������
  // 祣�-���� � ��᫥���� 9 ������ nextreply[]
  // ���⠭���� ���������.

  int ctrlsize = 1024;		// ������ ����� ��� �������
  char *ctrl;			// ����⢥��� �����. ���� �� ��।������. 8)

  strcpy (index_file, areaFile);
  strcat (index_file, ".sqi");
  strcpy (base_file, areaFile);
  strcat (base_file, ".sqd");

  // ����� �� �⥭�� �� ����頥� - ������ ��祣� � 䠩�� �� �㤥�.
  hi = sopen (index_file, O_RDONLY | O_BINARY, SH_DENYWR);
  if (hi == -1)
    {
      return 1;
    }
  // � �����, �� ��� �������, �⥭�� �㡨� ��䨣, ��᪮��� ����� � ��
  // �㤥� �����.
  hd = sopen (base_file, O_RDWR | O_BINARY, SH_DENYRW);
  if (hd == -1)
    {
      close (hi);
      return 2;
    }

  PROFILE (NULL);

  // ��⠥� ������� � ����砥� �᫮ p����� �p�����.
  fstat (hi, &idxst);
  if (idxst.st_size == 0)
    {
      close (hi);
      close (hd);
      return 0;
    }
  index = (sqifile *) myalloc (idxst.st_size, __FILE__, __LINE__);
  rd = read (hi, index, idxst.st_size);
  // ��, ��稭�� � �⮣� ������ 䠩� �����ᮢ ��� ����� �� �������.
  close (hi);
  if (rd != idxst.st_size)
    {
      // �� � �� ��ᨫ 400 ������, � ����� - 398! 8( 8)
      close (hd);
      return 3;
    }
  frames = idxst.st_size / sizeof (sqifile);

  // ����. � p��p����塞 ��� ���p���� � ���ᠣ�� ...
  msgs = (PMsgInfo) myalloc (sizeof (MsgInfo) * frames, __FILE__, __LINE__);
#ifdef MALLOC_DEBUG
  fprintf (stderr, "\nAllocated msgs for %lu frames\n", frames);
#endif

  // ������ �� �㦥� ����� ��� �������...
  // ����� � ����� ( ctrlsize + 1) ��᪮��� ctrlsize - �� _����_ ࠧ���
  // ������� ��� ��� ���楢��� 0.
  ctrl = (char *)myalloc (ctrlsize + 1, __FILE__, __LINE__);
  // ... � ��� ��� ࠡ���, ...
  msghash = createHash (frames, hfMSGID, cmpMSGID);
  umsghash = createHash (frames, hfumsgid, cmpumsgid);

  // ... � ������� ����� �� ����.
  int zero = 0;
  sqdmshead hdr;
  int subjoffs = (int)(hdr.subj - (char *)&hdr);
  int replyoffs = (int)((char *)&hdr.replyto - (char *)&hdr);
  int replysize = sizeof (unsigned long) * 11;
  unsigned long ident, clen;

  PROFILE ("reading of SQD started\n");

  for (i = 0; i < frames; i++)
    {

      if ((LINKFVERBOSESTEP > 0)
	  && (((i /* + 1 */ ) % LINKFVERBOSESTEP) == 0))
	{
	  ccprintf ("%s: read %-6lu of %-6lu\r", AREANAME, i, frames);
	}

      // ����뢠�� ����室��� ��᪨ ����������.
      lseek (hd, index[i].offset, SEEK_SET);
      rd = read (hd, &ident, sizeof (unsigned long));
      if ((rd != sizeof (unsigned long)) || (ident != SQHDRID))
	{
	  close (hd);
	  return 4;
	}
      lseek (hd, 16, SEEK_CUR);	// ���������� � clen
      rd = read (hd, &clen, sizeof (unsigned long));
      if (rd != sizeof (unsigned long))
	{
	  close (hd);
	  return 4;
	}
      if (linkType == LINKTYPE_SUBJ)
	{
	  char buf[SUBJ_MAXLEN + 1];
	  lseek (hd, 80, SEEK_CUR);	// ���������� � subj �᫨ ⨯ �������� ��� �p����.
	  // �뤥�塞 ���� ��� subj.
	  rd = read (hd, buf, SUBJ_MAXLEN + 1);
	  if (rd != (SUBJ_MAXLEN + 1))
	    {
	      close (hd);
	      return 4;
	    }
	  buf[SUBJ_MAXLEN] = 0;
	  int slen = strlen (buf) + 1;
	  // ����� ����� �뫮 �� ��ᯮ�짮������ strdup, �� �� �� �ᥣ��
	  // ������᭮, ����� �ᯮ������� �� �⠭��p�� �p���⢠ p��p��������
	  // �����. ���⮬� ���� ᤥ���� �� �p����, �� � dbg_* �� ����
	  // �p��� �p� p��p��������. 8)
	  msgs[i].subj = (char *)myalloc (slen, __FILE__, __LINE__);
	  memcpy (msgs[i].subj, buf, slen);
	  lseek (hd, 26, SEEK_CUR);	// ���������� � replyto
	}
      else
	{
	  // �� subj ��������� �� �㦭� - �p��� �p���᪠�� ᮮ�. ��᪨ �
	  // ���������.
	  lseek (hd, 178, SEEK_CUR);
	  msgs[i].subj = NULL;
	}
      rd = read (hd, &msgs[i].replyto, replysize);
      if (rd != replysize)
	{
	  close (hd);
	  return 4;
	}
      lseek (hd, 20, SEEK_CUR);
      // �᫨ ���� ࠧ��� ������� � ���쬥 �����, 祬 ���������, �
      // ����⨬ �����.
      if (clen > ctrlsize)
	{
	  ctrlsize = clen;
	  ctrl = (char *)myrealloc (ctrl, ctrlsize + 1, __FILE__, __LINE__);
	}
      rd = read (hd, ctrl, clen);
      if (rd != clen)
	{
	  close (hd);
	  return 6;
	}
      ctrl[clen] = 0;
      msgs[i].msgid = getMSGID (ctrl);
      msgs[i].reply_id = get_reply_id (ctrl);
      if ((msgs[i].reply_id != 0) && (msgs[i].reply_id == msgs[i].msgid))
	{
	  // �ࠩ�� ��࠭��� �����, ����� ���쬮 ���� �⢥⮬ �� ᠬ�� ᥡ�.
	  // ���⨬��, �⮡� �� ������ � ���� 横��.
	  msgs[i].reply_id = 0;
	}
      // ����� �� �ᯮ��㥬 ������� memcpy � �.�., ��᪮��� ⮫쪮
      // ��������� �㤥�.
      msgs[i].offset = index[i].offset;
      msgs[i].umsgid = index[i].umsgid;
      msgs[i].processed = msgs[i].changed = 0;
      msgs[i].next = msgs[i].prev = msgs[i].head = msgs[i].last = NULL;

      // �� ��砩 �᫨ ���� ⨯ �������� ��� ���� �� ������� � TimEd
      // �� �� ������� - ��⨬ �� netxreply �஬� ��ࢮ��. �⮡�
      // �� ���� ��⮬ �� ����� � ���� ᭠砫� �஢�ਬ - � ���� ��
      // ���㫥��� nextreply � 9 ��᫥����.
      if ((linkType != LINKTYPE_TIMED)
	  &&
	  (memcmp
	   (msgs[i].nextreply + 1, zero_array,
	    9 * sizeof (unsigned long)) != 0))
	{
	  memset (msgs[i].nextreply + 1, 0, sizeof (unsigned long) * 9);
	  msgs[i].mark_changed ();
	  zero++;
	}
      // � 䨭���� ����� ���⪨ - �����塞 � ���.
      insertElement (msghash, &msgs[i]);
      insertElement (umsghash, &msgs[i]);
    }
  if (LINKFVERBOSESTEP > 0)
    {
      ccprintf ("%s: read %-6lu of %-6lu\r", AREANAME, i, frames);
    }

  PROFILE ("reading of SQD finished\n");

  // � �������� ����� �� ������� - "���� ����� ��䨣".
  myfree ((void **)&ctrl, __FILE__, __LINE__);
  // ������� ⮦� ����� �� �����������.
  myfree ((void **)&index, __FILE__, __LINE__);

  // �⮣�, � �⮬� ������� ����� ���ᨢ ���ଠ樨 � ���ᠣ�� msgs, �����
  // ������ 墠��� ��� �������� �� �����, � ���, ������ 㪠��⥫ﬨ ��
  // �� ������ �⮣� ���ᨢ�.

  PROFILE (NULL);

  switch (linkType)
    {
    case LINKTYPE_SUBJ:
      linkSubj (msgs, frames);
      break;
    case LINKTYPE_REPLY:
    case LINKTYPE_FLAT:
      linkLinear (msghash, umsghash, msgs, frames, linkType);
      break;
    case LINKTYPE_TIMED:
      linkTimEd (msghash, umsghash, msgs, frames);
      break;
    case LINKTYPE_NONE:
      // �� ����, � �� �� ⮣�� ����� ������??? 8)
      break;
    }

  PROFILE ("linking finished\n");

  // �⫨���������. ��� ᢠ������ �� ����� � ����.
  for (i = 0; i < frames; i++)
    {
      if (msgs[i].changed)
	{
	  lseek (hd, msgs[i].offset + replyoffs, SEEK_SET);
	  // ��襬 ����� 11 unsigned long'��: replyto � 10 nextreply'��.
	  write (hd, &msgs[i].replyto, sizeof (unsigned long) * 11);
	}
    }

  PROFILE ("wrote changes back to SQD\n");

  destroyHash (msghash);
  destroyHash (umsghash);
  for (i = 0; i < frames; i++)
    {
      if (msgs[i].subj)
	{
	  myfree ((void **)&msgs[i].subj, __FILE__, __LINE__);
	}
    }
  myfree ((void **)&msgs, __FILE__, __LINE__);
  close (hd);

  PROFILE ("finished linking\n");

//    if ( LINKFVERBOSESTEP > 0) ccprintf( "%-78s\r%s: %lu of %lu\r", "", AREANAME, changed_count, frames);
  //if ( LINKFVERBOSESTEP > 0)
  if (!quiet || LINKFVERBOSESTEP > 0)
    ccprintf ("%-78s\r%s: %lu of %lu\r\n", "", AREANAME, changed_count,
	      frames);
  if (changed_count)
    {
      sprintf (logout, "Linking %s: %lu of %lu", AREANAME, changed_count,
	       frames);
      logwrite (1, 1);
    }

  return 0;
}

#ifdef STANDALONE_VERSION

#ifdef TRAP_CATCHING
void sigh (int sig)
{
  sig = sig;
  fprintf (stderr, "\nfailed at line %d...\n", last_line_passed);
  abort ();
}
#endif

int main (int argc, char *argv[])
{
  int linkType = LINKTYPE_TIMED;

#ifdef TRAP_CATCHING
  signal (SIGSEGV, sigh);
#endif

  if (argc < 2)
    {
      return 1;
    }
  if (argc > 2)
    {
      linkType = atoi (argv[2]);
    }
  return linkArea (argv[1], linkType);
}
#endif	/* STANDALONE_VERSION */
