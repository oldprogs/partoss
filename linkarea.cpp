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
#include <math.h>
#include <ctype.h>
#include <fcntl.h>
#if !defined (__LNX__) && !defined (__FreeBSD__)
#include <io.h>
#endif
#include <sys/timeb.h>
#include "globext.h"
#include "partoss.h"

#define LINKTYPE_REPLY          1
#define LINKTYPE_SUBJ           2
#define LINKTYPE_FLAT           3
#define LINKTYPE_TIMED          4
#define LINKTYPE_NONE           5

// ����� �᫮ ��������� �� ���祭�� �� ���䨣� �� ���ࠨ����� �
// �����. bcfg.linklength, ⠪ �������.
#define LINK_LENGTH             bcfg.linklength
// �᫨ subj � ��������� ����� �� ����稢����� 0, � ����� ���� ������ 72
#define SUBJ_MAXLEN             71

// myalloc � myfree - �६���� �����誨, �⮡� �� �ਢ�뢠���� � ������ ���栬.
// ��᪮�쪮 ������ �����ᢨ� myrealloc - ��⥭樠�쭠� �஡���� ��� ���-���ᨨ.
#define myalloc( sz, f, l)  malloc( sz)
#define myfree( pptr, f, l) free( *( pptr))
#define myrealloc( ptr, sz, f, l) realloc( ( ptr), ( sz))

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

/*
// time_diff � time_point ����� ���� �ᯮ�짮���� ��� ��䨫�஢����
// ��᪮� ����, ����� ��� �६��� ���� �� �����ᥪ㭤�.
float timeb_diff( struct timeb &t1, struct timeb &t2)
{
    float tfm = ( float( t1.millitm) - float( t2.millitm)) / 1000., tft = float( t1.time) - float( t2.time);
    return tfm + tft;
}

float time_point()
{
    static struct timeb pt;
    struct timeb curt;
    float diff;

    ftime( &curt);

    diff = timeb_diff( curt, pt);

    memcpy( &pt, &curt, sizeof( struct timeb));

    return diff;
}
*/

//=========================================================================
// Working code
//=========================================================================

// ��� ���������� ��� � ����⢥ ���� �����筮 �ᯮ�짮���� MSGID ᠬ
// �� ᥡ� - �� �����筮 ��砥�.

typedef struct MsgInfo
{
  sqdmshead hdr;		// ����⢥��� ��������� ���ᠣ�
  unsigned long iidx;		// ���饭�� � ���ᨢ� �����ᮢ
  // ����᫥���饥 ������ �� ������ - �⮡ ����� ᥡ� ��⮬ �� �����
  // � �離�� �����ᮢ.
  sqifile index_info;
  // ������ �, �� ���� ����� �ਣ�������.
  unsigned long msgid, reply_id;
  int changed;
  int processed;
  // ����� ���� ��� ⨯�� �������� Flat/Reply � Subj. � ��饬 - ���
  // ��������, ��� �㦭� ᯨ᪨.
  MsgInfo *next, *prev;		// �����⥫� �� ᫥���饥 � �।��饥 ���쬠 �
  // 楯�窥.
  MsgInfo *head;		// �����⥫� �� ������ ᯨ᪠.
  MsgInfo *last;		// �����⥫� �� ��᫥���� ����� � ᯨ᪥.
  // ����⢨⥫�� ⮫쪮 � ��������� ����� ᯨ᪠.
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
      // �᫨ �᫮ � ����� ����⥫� - � �� � ����� ���� �����⭮��
      // �⮣� �᫠.
      unsigned long divisor = (unsigned long)sqrt (i);
      // ������塞 ����⥫� ��� ������襥 ᢥ��� ���祭��, ���஥ �����⨬�
      // ��� ��ॡ�� ����⥫��. �����⨬� - ���� �� ��⭮ �� 3, �� 2.
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
      if (divisor <= 3)
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
  unsigned long hashSize = gen_prime (expectedElements * 3);

  if (expectedElements == 0)
    {
      return NULL;
    }

  newHash = (PHash) myalloc (sizeof (Hash), __FILE__, __LINE__);
  newHash->size = hashSize;
  newHash->dupcount = newHash->used = newHash->elements = 0;
  newHash->table =
    (PHashElem *) myalloc (hashSize * sizeof (PHashElem), __FILE__, __LINE__);
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
  return msgInfo->index_info.umsgid % hashSize;
}

int cmpumsgid (PMsgInfo m1, PMsgInfo m2)
{
  return m1->index_info.umsgid == m2->index_info.umsgid;
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

int subjLen (const char *subj, int *pos)
{
  *pos = cleanSubjPos (subj);
  return ((SUBJ_MAXLEN - *pos) <
	  LINK_LENGTH ? (SUBJ_MAXLEN - *pos) : LINK_LENGTH);
}

unsigned long hfSubj (unsigned long hashSize, PMsgInfo msgInfo)
{
  int pos, blen;
  unsigned long crc;

  blen = subjLen (msgInfo->hdr.subj, &pos);
  crc = crc32block (msgInfo->hdr.subj + pos, blen, (unsigned long)-1);

  return crc % hashSize;
}

int cmpSubj (PMsgInfo m1, PMsgInfo m2)
{
  int len1, pos1, len2, pos2;

  len1 = subjLen (m1->hdr.subj, &pos1);
  len2 = subjLen (m2->hdr.subj, &pos2);

  return (len1 == len2)
    && (strncmp (m1->hdr.subj + pos1, m2->hdr.subj + pos2, len1) == 0);
}

//-------------------------------------------------------------------------
// ��᪨ ࠡ��� � �����
//-------------------------------------------------------------------------

// str2MSGID - ������ strtoul. ����p�� ����砥���.
unsigned long str2MSGID (const char *str)
{
  unsigned long idval = 0;
  int i;
  for (i = 0; i < 8 && isalnum (str[i]); i++)
    {
      idval =
	idval * 16 + (isalpha (str[i]) ? toupper (str[i]) - 'A' +
		      10 : str[i] - '0');
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
  p += 8;
  p = strchr (p, ' ') + 1;
  // p ����� ��� p�� ����� �� ��砫� ᮡ�⢥��� ID. ���砩 ����� MSGID
  // �� p��ᬠ�p�������, ��� �� �㡠� �� ����� ���� ����⠫쭮, �᫨
  // ����� �p���p��� ��� ���������.
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
  p += 8;
  p = strchr (p, ' ') + 1;
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

  for (i = 0; i < frames; i++)
    {
      PMsgInfo rmsg;		// � �㥬 �᪠�� ���쬮, �� ���஥ ⥪�饥 - �⢥�.
      if (msgs[i].hdr.nextreply[0] != 0)
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
	  tmpmsg.index_info.umsgid = msgs[i].hdr.nextreply[0];
	  PMsgInfo nextmsg = findElement (umsghash, &tmpmsg);
	  if ((!nextmsg) || (nextmsg->reply_id != msgs[i].msgid))
	    {
	      msgs[i].hdr.nextreply[0] = 0;
	      msgs[i].changed = 1;
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
	  if (msgs[i].hdr.replyto != 0)
	    {
	      // � ࠭��, ��宦�, �뫠 - replyto ��࠭����. ����� -
	      // ���뢠�� � 0 � ������ ������.
	      msgs[i].hdr.replyto = 0;
	      msgs[i].changed = 1;
	    }
	  continue;
	}
      // � ⥯��� ���� �� ����: ���� �� ��� ���ᠣ� 㦥 �뫨 ᫨�������,
      // ���� �� �� �।�⮨� ᤥ���� - � ⮣�� �㦭� ���� � nextreply
      // �⮡� � ���� �ய���� ��뫪�.
      firstempty = -1;
      linked = 0;
      if (msgs[i].hdr.replyto != rmsg->index_info.umsgid)
	{
	  // �������, ���ᠣ� �� �� �����������. ��� ��-� �����
	  // ���������. ��� ࠭�� ����������� ����. � �� ��砥 -
	  // ���⠢�塞 replyto.
	  msgs[i].hdr.replyto = rmsg->index_info.umsgid;
	  msgs[i].changed = 1;
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
	  if (msgs[i].index_info.umsgid == rmsg->hdr.nextreply[j])
	    {
	      // ����� �த������ ��� ��᫠ - ��� "�裡��� 㦥 �뫨", �
	      // ����� ������...
	      linked = 1;
	      break;
	    }
	  if (firstempty < 0 && rmsg->hdr.nextreply[j] == 0)
	    {
	      firstempty = j;
	    }
	}
      if (!linked && firstempty >= 0)
	{
	  rmsg->hdr.nextreply[firstempty] = msgs[i].index_info.umsgid;
	  rmsg->changed = 1;
	}
      // ��⮢�. � �⮬� ������� �� ���易��...
    }
}

//-------------------------------------------------------------------------
// ��������� �������� ⨯�� �������� ��� @MSGID/@REPLY.
//-------------------------------------------------------------------------

// �஡���� �� ��������� ���쬠 �� ������ � ��뢠��� ���ᠣ � ᯨ᮪ �⢥⮢.
// �।����������, �� �������� ���쬮 ���� ������� ᯨ᪠.
void markList (PHash msghash, PHash umsghash, PMsgInfo msg)
{
  PMsgInfo curMsg = msg;
  MsgInfo tmpmsg;
  msg->head = msg;
  msg->last = msg;
  while (curMsg && (curMsg->hdr.nextreply[0] != 0))
    {
      tmpmsg.index_info.umsgid = curMsg->hdr.nextreply[0];
      curMsg->next = findElement (umsghash, &tmpmsg);
      if (curMsg->next)
	{
	  if (curMsg->next->reply_id == 0)
	    {
	      // ������ - ��� �� ᯨ�� �� ������ 楯�窨 �⢥⮢! ����
	      // ������ - �㡨�� ⠪�� ��뫪� � ����.
	      curMsg->hdr.nextreply[0] = 0;
	      curMsg->changed = 1;
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
		  curMsg->next->hdr.nextreply[0] = 0;
		}
	      if ((curMsg->next->prev != curMsg) ||
		  (curMsg->next->next == curMsg) || (curMsg->next == curMsg))
		{
		  curMsg->hdr.nextreply[0] = 0;
		  curMsg->changed = 1;
		  if (curMsg->next->next == curMsg)
		    {
		    }
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

  // �� �㬠�, �⮡� �᫮ 㡨��� ��ᥬ, �� ����� ���� �⢥��, �뫮 �����
  // �⢥�� �������� � ����. ���॥ �ᥣ� - �� �������� � 10-� ���.
  // �� �᫨ �� �� ���� ⠪��� ���ᨢ� �� 墠�� - �����।���� �� ����室�����.
  // fakechunk ������ ��� ���������� �����।������ �⮡� ����� ������
  // ᥡ� ������ - ������ ��᪠�� ���騢��� ���ᨢ 䥩���.
  if (linkType == LINKTYPE_REPLY)
    {
      fakechunk = frames < 4 ? frames : frames / 4;
      fakemsgs =
	(PMsgInfo *) myalloc (sizeof (MsgInfo) * fakesize, __FILE__,
			      __LINE__);
    }

  for (i = 0; i < frames; i++)
    {
      PMsgInfo rmsg, curMsg;

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
	  markList (msghash, umsghash, msgs + i);
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
		  last->hdr.nextreply[0] = curMsg->index_info.umsgid;
		  last->changed = 1;
		  curMsg->hdr.replyto = last->index_info.umsgid;
		  curMsg->changed = 1;

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

	      if (curMsg->hdr.replyto != rmsg->index_info.umsgid)
		{
		  // ���쬮 �� �� ����������� ��� �뫮 ᫨������� �����४⭮.
		  // �� ���� 䨪���.
		  curMsg->hdr.replyto = rmsg->index_info.umsgid;
		  curMsg->changed = 1;
		}
	      if (rmsg->hdr.nextreply[0] != curMsg->index_info.umsgid)
		{
		  // ���� � �� 䨣��: ���� � nextreply ᨤ�� ���⪨
		  // ࠭�� �ᯮ�짮���襣��� ⨯� ��������, ���� ���쬮
		  // ����� �� �� �����࣠���� ��ࠡ�⪥.
		  rmsg->hdr.nextreply[0] = curMsg->index_info.umsgid;
		  rmsg->changed = 1;
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
      if (curMsg->hdr.replyto != 0)
	{
	  // ���⨬ �������� ���⪨ ��᫥ ᬥ�� ⨯� ��������.
	  // � ᠬ�� ����: ����� ����� ���� replyto � ������ ᯨ᪠?
	  curMsg->hdr.replyto = 0;
	  curMsg->changed = 1;
	}
      if (linkType == LINKTYPE_FLAT || curMsg->reply_id == 0)
	{
	  // �᫨ Flat - � �ਭ����� ���ᠣ� �� ������ � ��ந� �� �⮩
	  // ������ ���� ᯨ᮪.
	  markList (msghash, umsghash, curMsg);
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
		(PMsgInfo *) myrealloc (fakemsgs, fakesize, __FILE__,
					__LINE__);
	    }
	  fakemsgs[fakecount] =
	    (PMsgInfo) myalloc (sizeof (MsgInfo), __FILE__, __LINE__);
	  memset (fakemsgs[fakecount], 0, sizeof (MsgInfo));

	  fakemsgs[fakecount]->processed = 1;
	  fakemsgs[fakecount]->msgid = curMsg->reply_id;

	  fakemsgs[fakecount]->hdr.nextreply[0] = curMsg->index_info.umsgid;

	  // ������ ��� 䨪⨢ �㦥� ��� � ���. ��筥� - ����� �����������
	  // �����, ��� �離� � �� @REPLY.
	  insertElement (msghash, fakemsgs[fakecount]);

	  markList (msghash, umsghash, fakemsgs[fakecount]);

	  fakecount++;
	}
    }

  // ����頥� �� ᮡ�� ������...
  if (linkType == LINKTYPE_REPLY)
    {
      for (i = 0; i < fakecount; i++)
	{
	  myfree ((void **)&fakemsgs[i], __FILE__, __LINE__);
	}
      myfree (&fakemsgs, __FILE__, __LINE__);
    }
}

//-------------------------------------------------------------------------
// ��������� �������� �� Subj
//-------------------------------------------------------------------------

void linkSubj (PHash msghash, PHash umsghash, PMsgInfo msgs,
	       unsigned long frames)
{
  unsigned long i;
  // ��� ��� ����� ᯨ᪮� �������� �� Subj.
  PHash smsghash;

  // �����뢠��, �� ᯨ᪮� ������ �� Subj �㤥� �� �����, 祬 ���� ��
  // ��饣� �᫠ ��ᥬ.
  smsghash = createHash (frames / 3, hfSubj, cmpSubj);

  for (i = 0; i < frames; i++)
    {
      PMsgInfo hmsg, curMsg;
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
      hmsg = findElement (smsghash, curMsg);
      if (hmsg != NULL)
	{
	  PMsgInfo last = hmsg->last;

	  curMsg->head = hmsg;
	  curMsg->prev = last;
	  last->next = curMsg;

	  if (curMsg->hdr.replyto != last->index_info.umsgid)
	    {
	      // ���쬮 �� �� ����������� ��� �뫮 ᫨������� �����४⭮.
	      // �� ���� 䨪���.
	      curMsg->hdr.replyto = last->index_info.umsgid;
	      curMsg->changed = 1;
	    }
	  if (last->hdr.nextreply[0] != curMsg->index_info.umsgid)
	    {
	      // ���� � �� 䨣��: ���� � nextreply ᨤ�� ���⪨
	      // ࠭�� �ᯮ�짮���襣��� ⨯� ��������, ���� ���쬮
	      // ����� �� �� �����࣠���� ��ࠡ�⪥.
	      last->hdr.nextreply[0] = curMsg->index_info.umsgid;
	      last->changed = 1;
	    }

	  hmsg->last = curMsg;
	}
      else
	{
	  // ���᪠ � ⠪�� subj ��宦� ���. ����� �㤥� ��稭��� ����.

	  insertElement (smsghash, curMsg);

	  curMsg->head = curMsg;
	  curMsg->last = curMsg;
	  curMsg->hdr.replyto = 0;
	  curMsg->hdr.nextreply[0] = 0;
	  curMsg->changed = 1;
	  // ����� ������, ���樠������ head ��� �������� �� subj ������,
	  // �� ���� �� �����, �᫨ ����������� � ���饬.
	}
    }

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

  // ��⠥� ������� � ����砥� �᫮ p����� �p�����.
  fstat (hi, &idxst);
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

  // ������ �� �㦥� ����� ��� �������...
  // ����� � ����� ( ctrlsize + 1) ��᪮��� ctrlsize - �� _����_ ࠧ���
  // ������� ��� ��� ���楢��� 0.
  ctrl = (char *)myalloc (ctrlsize + 1, __FILE__, __LINE__);
  // ... � ��� ��� ࠡ���, ...
  msghash = createHash (frames, hfMSGID, cmpMSGID);
  umsghash = createHash (frames, hfumsgid, cmpumsgid);

  // ... � ������� ����� �� ����.
  int zero = 0;
  for (i = 0; i < frames; i++)
    {
      lseek (hd, index[i].offset, SEEK_SET);
      // ���⠥� ��������� �३�� ������ � ���������� ���ᠣ�. ��᪮���
      // � ������� sqdmshead ���� text �⭮���� 㦥 � �������, � ���
      // �� ��饣� ࠧ��� ����뢠��. � ����� � 楫�� ������ �ᯮ�짮����
      // ��� �� �㤥�.
      rd = read (hd, &msgs[i].hdr, sizeof (sqdmshead) - sizeof (char *));
      if (rd != sizeof (sqdmshead) - sizeof (char *))
	{
	  close (hd);
	  return 4;
	}
      if (msgs[i].hdr.ident != SQHDRID)
	{
	  close (hd);
	  return 5;
	}
      // �᫨ ���� ࠧ��� ������� � ���쬥 �����, 祬 ���������, �
      // ����⨬ �����.
      if (msgs[i].hdr.clen > ctrlsize)
	{
	  ctrlsize = msgs[i].hdr.clen;
	  // ����, myrealloc �� �ਣ������, IMHO. ����-��...
	  ctrl = (char *)myrealloc (ctrl, ctrlsize + 1, __FILE__, __LINE__);
	}
      rd = read (hd, ctrl, msgs[i].hdr.clen);
      if (rd != msgs[i].hdr.clen)
	{
	  close (hd);
	  return 6;
	}
      ctrl[msgs[i].hdr.clen] = 0;
      // � ����娢��� ⥯��� ��, �� ����� ���᫥��⢨� �ਣ�������.
      msgs[i].msgid = getMSGID (ctrl);
      msgs[i].reply_id = get_reply_id (ctrl);
      // ����� �� �ᯮ��㥬 ������� memcpy � �.�., ��᪮��� ⮫쪮
      // ��������� �㤥�.
      msgs[i].index_info.offset = index[i].offset;
      msgs[i].index_info.umsgid = index[i].umsgid;
      msgs[i].index_info.hash = index[i].hash;
      msgs[i].processed = msgs[i].changed = 0;
      msgs[i].next = msgs[i].prev = msgs[i].head = msgs[i].last = NULL;
      // iidx ����� � �� �����������, ��᪮��� ���ଠ�� �� ������ ���
      // � ⠪ ��������.
      msgs[i].iidx = i;
      // �� ��砩 �᫨ ���� ⨯ �������� ��� ���� �� ������� � TimEd
      // �� �� ������� - ��⨬ �� netxreply �஬� ��ࢮ��. �⮡�
      // �� ���� ��⮬ �� ����� � ���� ᭠砫� �஢�ਬ - � ���� ��
      // ���㫥��� nextreply � 9 ��᫥����.
      if ((linkType != LINKTYPE_TIMED)
	  &&
	  (memcmp
	   (msgs[i].hdr.nextreply + 1, zero_array,
	    9 * sizeof (unsigned long)) != 0))
	{
	  memset (msgs[i].hdr.nextreply + 1, 0, sizeof (unsigned long) * 9);
	  msgs[i].changed = 1;
	  zero++;
	}
      // � 䨭���� ����� ���⪨ - �����塞 � ���.
      insertElement (msghash, &msgs[i]);
      insertElement (umsghash, &msgs[i]);
    }
  // � �������� ����� �� ������� - "���� ����� ��䨣".
  myfree (&ctrl, __FILE__, __LINE__);

  // �⮣�, � �⮬� ������� ����� ���ᨢ ���ଠ樨 � ���ᠣ�� msgs, �����
  // ������ 墠��� ��� �������� �� �����, � ���, ������ 㪠��⥫ﬨ ��
  // �� ������ �⮣� ���ᨢ�.

  switch (linkType)
    {
    case LINKTYPE_SUBJ:
      linkSubj (msghash, umsghash, msgs, frames);
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

  // �⫨���������. ��� ᢠ������ �� ����� � ����.
  for (i = 0; i < frames; i++)
    {
      if (msgs[i].changed)
	{
	  lseek (hd, msgs[i].index_info.offset, SEEK_SET);
	  // � ࠧ��஬ � �� �����, �� � �� ���⪥ - ���� text
	  // ����뢠�� �� ����譮���� � �।������ - �⮡ ����
	  // � ���� �� ������.
	  write (hd, &msgs[i].hdr, sizeof (sqdmshead) - sizeof (char *));
	}
    }

  destroyHash (msghash);
  destroyHash (umsghash);
  myfree (&index, __FILE__, __LINE__);
  myfree (&msgs, __FILE__, __LINE__);

  return 0;
}

/*
// ������让 ��⮢� ��᮪ - ��� ��� �஢�ન ���. ���� � stdin ᯨ᮪
// MSGID � �ண���� �� �⮬ ᯨ᪥ 䨪⨢�� ���.
void main()
{
    static unsigned long msgids[ 100000];
    unsigned long n = 0, i;
    char buf[ 32];
    PMsgInfo msgs, msgInfo;
    PHash hash;
    float t1, t2, t3;

    while ( !feof( stdin)) {
        scanf( "%s", buf);
        msgids[ n++] = str2MSGID( buf);
    }

    hash = createHash( n, hfMSGID, cmpMSGID);

    msgs = ( PMsgInfo) myalloc( sizeof( MsgInfo) * n, __FILE__, __LINE__);
    time_point();
    for ( i = 0; i < n; i++) {
        msgs[ i].msgid = msgids[ i];
        msgs[ i].iidx = i;
        insertElement( hash, &msgs[ i]);
    }
    t1 = time_point();

    ccprintf( "Time used to fill: %f\n", t1);
    ccprintf( "Size: %lu, elements: %lu, used: %lu\n", hash->size, hash->elements, hash->used);

    // �஢�ਬ १�����. ���砫� - �஢�ਬ, �� �� MSGID ��室��� ���४⭮.
    MsgInfo tmpmsg;
    for ( i = 0; i < n; i++) {
        tmpmsg.msgid = msgids[ i];
        msgInfo = findElement( hash, &tmpmsg);
        if ( msgInfo == NULL || msgInfo->iidx != i) {
            ccprintf( "Check failed for %08x\n", msgids[ i]);
        }
    }

    // ������ - ⠩�����.

    // ��� ��砫� - ����稬 ��᫥����⥫쭮��� ��砩��� �����ᮢ.
    #define SEQUENCE_LEN 1000000
    static int ridx[ SEQUENCE_LEN];
    for ( i = 0; i < SEQUENCE_LEN; i++) {
        ridx[ i] = rand() % n;
    }

    // ������ ᬮ�ਬ, ᪮�쪮 �६��� �������� ���饭�� � ���-⠡���.
    // ��� ��᮫�⭮� ����� ������ �� ��室�. ���� ��� ����� ���
    // �६�, �㦭�� �� ��ࠡ��� �஢�ન: ࠧ��� �६�� ��ࢮ�� �
    // ��ண� 横��� ���� ��⮥ �६�, ����稢����� �� �஢��� �᫮���,
    // � �६�, ����祭��� �� ᠬ 横�, ⠪ � � ����, 8) �ந�����㥬.
    MsgInfo mi;
    msgInfo = &mi;
    time_point();
    for ( i = 0; i < SEQUENCE_LEN; i++) {
        tmpmsg.msgid = msgids[ ridx[ i]];
        msgInfo->iidx = ridx[ i];
    }
    t1 = time_point();
    for ( i = 0; i < SEQUENCE_LEN; i++) {
        tmpmsg.msgid = msgids[ ridx[ i]];
        msgInfo->iidx = ridx[ i];
        if ( msgInfo == NULL || msgInfo->iidx != ridx[ i]) {
            ccprintf( "Test lookup failed for %08x\n", msgids[ ridx[ i]]);
        }
    }
    t2 = time_point();
    for ( i = 0; i < SEQUENCE_LEN; i++) {
        tmpmsg.msgid = msgids[ ridx[ i]];
        msgInfo = findElement( hash, &tmpmsg);
        if ( msgInfo == NULL || msgInfo->iidx != ridx[ i]) {
            ccprintf( "Timing test failed for %08x\n", msgids[ ridx[ i]]);
        }
    }
    t3 = time_point();
    ccprintf( "Time used for %lu findElements: %f. I.e. %lf per element\n", SEQUENCE_LEN, t3 - ( t2 - t1), double( t3 - ( t2 - t1)) / SEQUENCE_LEN);
    ccprintf( "This excludes %f seconds used for condition check\n", t2 - t1);
}
*/

int main (int argc, char *argv[])
{
  int linkType = LINKTYPE_TIMED;
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
